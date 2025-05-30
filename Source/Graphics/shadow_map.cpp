#include "Graphics/Renderer.hpp"
#include "World.hpp"

GfxTexture g_shadow_map_texture;
GfxSamplerState g_shadow_map_sampler;
GfxTexture g_shadow_map_noise_texture;
GfxSamplerState g_shadow_map_noise_sampler;

float g_shadow_map_cascade_sizes[Shadow_Map_Num_Cascades] = {5, 20, 50, 200};
float g_shadow_map_depth_extent_factor = 6;
float g_shadow_map_forward_offset = 1;

float g_shadow_map_min_depth_bias = 0.2;
float g_shadow_map_max_depth_bias = 0.5;
float g_shadow_map_normal_bias = 150;
float g_shadow_map_filter_radius = 1;

Mat4f GetShadowMapCascadeMatrix(Vec3f light_direction, Mat4f camera_transform, int level)
{
    float size = g_shadow_map_cascade_sizes[level];
    float depth = size * g_shadow_map_depth_extent_factor;
    Vec3f center = TranslationVector(camera_transform)
        + ForwardVector(camera_transform) * size * 0.5 * g_shadow_map_forward_offset;

    Mat4f light_projection = Mat4fOrthographicProjection(-size * 0.5, size * 0.5, -size * 0.5, size * 0.5, -depth * 0.5, depth * 0.5);

    Mat4f light_view = Mat4fLookAt(center, center + light_direction, {1,0,0});
    light_view = Inverted(light_view);

    return light_projection * light_view;
}

void InitShadowMap()
{
    GfxTextureDesc noise_desc = {};
    noise_desc.type = GfxTextureType_Texture2DArray;
    noise_desc.pixel_format = GfxPixelFormat_RGBAFloat32;
    noise_desc.width = Shadow_Map_Noise_Size;
    noise_desc.height = Shadow_Map_Noise_Size;
    noise_desc.array_length = Shadow_Map_Num_Filtering_Samples;
    noise_desc.usage = GfxTextureUsage_ShaderRead;
    noise_desc.cpu_access = GfxCpuAccess_Write;
    g_shadow_map_noise_texture = GfxCreateTexture("Shadow Map Noise", noise_desc);

    RNG rng = {};
    RandomSeed(&rng, 348967956);

    Vec4f *noise_pixels = Alloc<Vec4f>(Shadow_Map_Noise_Size * Shadow_Map_Noise_Size * Shadow_Map_Num_Filtering_Samples, heap);
    for (int i = 0; i < Shadow_Map_Num_Filtering_Samples; i += 1)
    {
        for (int y = 0; y < Shadow_Map_Noise_Size; y += 1)
        {
            for (int x = 0; x < Shadow_Map_Noise_Size; x += 1)
            {
                Vec2f offset = {RandomGetZeroToOnef(&rng), RandomGetZeroToOnef(&rng)};

                auto pixel = &noise_pixels[i * Shadow_Map_Noise_Size * Shadow_Map_Noise_Size + y * Shadow_Map_Noise_Size + x];
                pixel->x = sqrtf(offset.y) * cosf(2 * Pi * offset.x);
                pixel->y = sqrtf(offset.y) * sinf(2 * Pi * offset.x);

                offset = {RandomGetZeroToOnef(&rng), RandomGetZeroToOnef(&rng)};
                pixel->z = sqrtf(offset.y) * cosf(2 * Pi * offset.x);
                pixel->w = sqrtf(offset.y) * sinf(2 * Pi * offset.x);
            }
        }
    }

    for (int i = 0; i < Shadow_Map_Num_Filtering_Samples; i += 1)
    {
        GfxReplaceTextureRegion(
            &g_shadow_map_noise_texture,
            {0,0,0}, {Shadow_Map_Noise_Size, Shadow_Map_Noise_Size, 1},
            0, (u32)i,
            noise_pixels + i * Shadow_Map_Noise_Size * Shadow_Map_Noise_Size
        );
    }

    GfxSamplerStateDesc sampler_desc = {};
    sampler_desc.min_filter = GfxSamplerFilter_Linear;
    sampler_desc.mag_filter = GfxSamplerFilter_Linear;
    sampler_desc.u_address_mode = GfxSamplerAddressMode_ClampToEdge;
    sampler_desc.v_address_mode = GfxSamplerAddressMode_ClampToEdge;
    sampler_desc.compare_func = GfxCompareFunc_Greater;
    g_shadow_map_sampler = GfxCreateSamplerState("Shadow Map", sampler_desc);

    sampler_desc = {};
    sampler_desc.min_filter = GfxSamplerFilter_Nearest;
    sampler_desc.mag_filter = GfxSamplerFilter_Nearest;
    sampler_desc.u_address_mode = GfxSamplerAddressMode_Repeat;
    sampler_desc.v_address_mode = GfxSamplerAddressMode_Repeat;
    sampler_desc.w_address_mode = GfxSamplerAddressMode_Repeat;
    g_shadow_map_noise_sampler = GfxCreateSamplerState("Shadow Map Noise", sampler_desc);

    RecreateShadowMapTexture(Shadow_Map_Default_Resolution);
}

void RecreateShadowMapTexture(u32 resolution)
{
    if (!IsNull(&g_shadow_map_texture))
        GfxDestroyTexture(&g_shadow_map_texture);

    GfxTextureDesc desc = {};
    desc.type = GfxTextureType_Texture2DArray;
    desc.pixel_format = GfxPixelFormat_DepthFloat32;
    desc.width = resolution;
    desc.height = resolution;
    desc.array_length = Shadow_Map_Num_Cascades;
    desc.usage = GfxTextureUsage_ShaderRead | GfxTextureUsage_DepthStencil;
    g_shadow_map_texture = GfxCreateTexture("Shadow Map", desc);
}

static GfxPipelineState g_shadow_map_pipeline;

static void InitShadowMapPipeline()
{
    GfxPipelineStateDesc desc = {};
    desc.depth_format = GfxPixelFormat_DepthFloat32;
    desc.depth_state = {.enabled=true, .write_enabled=true};
    desc.vertex_shader = GetVertexShader("shadow_map_geometry");
    desc.vertex_layout = MakeBlockVertexLayout();
    g_shadow_map_pipeline = GfxCreatePipelineState("Shadow Map", desc);
}

void ShadowMapPass(FrameRenderContext *ctx)
{
    if (IsNull(&g_shadow_map_pipeline))
    {
        InitShadowMapPipeline();
        Assert(!IsNull(&g_shadow_map_pipeline));
    }

    GfxRenderPassDesc pass_desc = {};
    pass_desc.render_target_array_length = Shadow_Map_Num_Cascades;
    GfxSetDepthAttachment(&pass_desc, &g_shadow_map_texture);
    GfxClearDepth(&pass_desc, 1);

    u32 resolution = GetDesc(&g_shadow_map_texture).width;
    auto pass = GfxBeginRenderPass("Shadow Map", ctx->cmd_buffer, pass_desc);
    {
        GfxSetPipelineState(&pass, &g_shadow_map_pipeline);
        GfxSetViewport(&pass, {.width=(float)resolution, .height=(float)resolution});

        auto vertex_frame_info = GfxGetVertexStageBinding(&g_shadow_map_pipeline, "frame_info_buffer");

        GfxSetBuffer(&pass, vertex_frame_info, FrameDataBuffer(), ctx->frame_info_offset, sizeof(Std140FrameInfo));

        foreach (i, ctx->world->all_chunks)
        {
            auto chunk = ctx->world->all_chunks[i];
            if (!chunk->mesh.uploaded)
                continue;

            GfxSetVertexBuffer(&pass, Default_Vertex_Buffer_Index, &chunk->mesh.vertex_buffer, 0, sizeof(BlockVertex) * chunk->mesh.vertex_count, sizeof(BlockVertex));

            GfxDrawIndexedPrimitives(&pass, &chunk->mesh.index_buffer, chunk->mesh.index_count, GfxIndexType_Uint32, Shadow_Map_Num_Cascades);
        }
    }
    GfxEndRenderPass(&pass);
}

