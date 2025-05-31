#include "Graphics/Renderer.hpp"
#include "World.hpp"
#include "UI.hpp"

#define Max_Chunk_Upload_Per_Frame 50

static GfxAllocator g_frame_data_allocators[Gfx_Max_Frames_In_Flight];

bool g_show_debug_atlas = false;

GfxAllocator *FrameDataGfxAllocator()
{
    return &g_frame_data_allocators[GfxGetBackbufferIndex()];
}

GfxBuffer *FrameDataBuffer()
{
    return &FrameDataGfxAllocator()->buffer;
}

Allocator FrameDataAllocator()
{
    return MakeAllocator(FrameDataGfxAllocator());
}

Allocator MakeAllocator(GfxAllocator *allocator)
{
    return {
        .data=allocator,
        .func=GfxAllocatorFunc,
    };
}

void InitGfxAllocator(GfxAllocator *allocator, String name, s64 capacity)
{
    GfxBufferDesc desc{};
    desc.cpu_access = GfxCpuAccess_Write;
    desc.size = capacity;
    allocator->buffer = GfxCreateBuffer(name, desc);
    Assert(!IsNull(&allocator->buffer));

    allocator->capacity = capacity;
    allocator->mapped_ptr = GfxMapBuffer(&allocator->buffer, 0, capacity, GfxMapAccess_Write);
    Assert(allocator->mapped_ptr != null);
}

void FlushGfxAllocator(GfxAllocator *allocator)
{
    GfxFlushMappedBuffer(&allocator->buffer, 0, allocator->top);
}

void ResetGfxAllocator(GfxAllocator *allocator)
{
    allocator->top = 0;
}

s64 GetBufferOffset(GfxAllocator *allocator, void *ptr)
{
    ptrdiff_t diff = (intptr_t)ptr - (intptr_t)allocator->mapped_ptr;
    if (diff < 0 || diff > allocator->capacity)
        return -1;

    return (s64)diff;
}

void *GfxAllocatorFunc(AllocatorOp op, s64 size, void *ptr, void *data)
{
    GfxAllocator *alloc = (GfxAllocator *)data;

    size = AlignForward(size, GfxGetBufferAlignment());

    switch (op)
    {
    case AllocatorOp_Alloc: {
        if (alloc->top + size > alloc->capacity)
            return null;

        void *ptr = (u8 *)alloc->mapped_ptr + alloc->top;
        alloc->top += size;

        return ptr;
    } break;

    case AllocatorOp_Free: break;
    }

    return null;
}

#define Frame_Data_Allocator_Capacity (1 * 1024 * 1024)

static GfxPipelineState g_post_processing_pipeline;
static GfxPipelineState g_chunk_pipeline;
static GfxTexture g_main_color_texture;
static GfxTexture g_main_depth_texture;
static GfxSamplerState g_linear_sampler;
static GfxSamplerState g_block_sampler;

void InitChunkMeshUploader();

Slice<GfxVertexInputDesc> MakeBlockVertexLayout()
{
    Array<GfxVertexInputDesc> vertex_layout = {.allocator=heap};
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_Float3,
        .offset=offsetof(BlockVertex, position),
        .stride=sizeof(BlockVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_UInt,
        .offset=offsetof(BlockVertex, block),
        .stride=sizeof(BlockVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_UInt,
        .offset=offsetof(BlockVertex, block_face),
        .stride=sizeof(BlockVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_UInt,
        .offset=offsetof(BlockVertex, block_corner),
        .stride=sizeof(BlockVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_UInt,
        .offset=offsetof(BlockVertex, occlusion),
        .stride=sizeof(BlockVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });

    return MakeSlice(vertex_layout);
}

void InitRenderer()
{
    LoadAllTextures();

    for (int i = 0; i < Gfx_Max_Frames_In_Flight; i += 1)
        InitGfxAllocator(&g_frame_data_allocators[i], TPrintf("Frame Data Allocator %d", i), Frame_Data_Allocator_Capacity);

    InitChunkMeshUploader();

    InitShadowMap();

    {
        GfxPipelineStateDesc pipeline_desc{};
        pipeline_desc.vertex_shader = GetVertexShader("post_processing");
        pipeline_desc.fragment_shader = GetFragmentShader("post_processing");
        pipeline_desc.color_formats[0] = GfxGetSwapchainPixelFormat();

        g_post_processing_pipeline = GfxCreatePipelineState("Post Processing", pipeline_desc);
        Assert(!IsNull(&g_post_processing_pipeline));
    }

    {
        GfxPipelineStateDesc pipeline_desc{};
        pipeline_desc.vertex_shader = GetVertexShader("mesh_geometry");
        pipeline_desc.fragment_shader = GetFragmentShader("mesh_geometry");
        pipeline_desc.color_formats[0] = GfxPixelFormat_RGBAFloat32;
        // pipeline_desc.rasterizer_state = {.fill_mode=GfxFillMode_Lines};
        pipeline_desc.blend_states[0] = {.enabled=true};
        pipeline_desc.depth_format = GfxPixelFormat_DepthFloat32;
        pipeline_desc.depth_state = {.enabled=true, .write_enabled=true};
        pipeline_desc.vertex_layout = MakeBlockVertexLayout();

        g_chunk_pipeline = GfxCreatePipelineState("Chunk", pipeline_desc);
        Assert(!IsNull(&g_chunk_pipeline));
    }

    {
        GfxSamplerStateDesc desc{};
        desc.min_filter = GfxSamplerFilter_Linear;
        desc.mag_filter = GfxSamplerFilter_Linear;
        desc.u_address_mode = GfxSamplerAddressMode_ClampToEdge;
        desc.v_address_mode = GfxSamplerAddressMode_ClampToEdge;
        g_linear_sampler = GfxCreateSamplerState("Linear", desc);
    }

    {
        GfxSamplerStateDesc desc{};
        desc.min_filter = GfxSamplerFilter_Linear;
        desc.mip_filter = GfxSamplerFilter_Linear;
        desc.u_address_mode = GfxSamplerAddressMode_ClampToEdge;
        desc.v_address_mode = GfxSamplerAddressMode_ClampToEdge;
        desc.w_address_mode = GfxSamplerAddressMode_ClampToEdge;
        g_block_sampler = GfxCreateSamplerState("Block", desc);
    }
}

static void RecreateRenderTargets()
{
    int width, height;
    SDL_GetWindowSizeInPixels(g_window, &width, &height);

    if (width <= 0 || height <= 0)
        return;

    auto main_color_desc = GetDesc(&g_main_color_texture);
    if (main_color_desc.width != (u32)width || main_color_desc.height != (u32)height)
    {
        if (!IsNull(&g_main_color_texture))
            GfxDestroyTexture(&g_main_color_texture);

        GfxTextureDesc desc{};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAFloat32;
        desc.width = width;
        desc.height = height;
        desc.usage = GfxTextureUsage_ShaderRead | GfxTextureUsage_RenderTarget;
        g_main_color_texture = GfxCreateTexture("Main Color", desc);
        Assert(!IsNull(&g_main_color_texture));
    }

    auto main_depth_desc = GetDesc(&g_main_depth_texture);
    if (main_depth_desc.width != (u32)width || main_depth_desc.height != (u32)height)
    {
        if (!IsNull(&g_main_depth_texture))
            GfxDestroyTexture(&g_main_depth_texture);

        GfxTextureDesc desc{};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_DepthFloat32;
        desc.width = width;
        desc.height = height;
        desc.usage = GfxTextureUsage_DepthStencil;
        g_main_depth_texture = GfxCreateTexture("Main Depth", desc);
        Assert(!IsNull(&g_main_depth_texture));
    }
}

void HandleChunkMeshGeneration(World *world);
void UploadPendingChunkMeshes(GfxCopyPass *pass, int max_uploads);

void RenderGraphics(World *world)
{
    FrameRenderContext ctx = {};
    ctx.world = world;

    int window_w, window_h;
    SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

    RecreateRenderTargets();

    GfxBeginFrame();
    GfxCommandBuffer cmd_buffer = GfxCreateCommandBuffer("Frame");
    ctx.cmd_buffer = &cmd_buffer;

    ResetGfxAllocator(FrameDataGfxAllocator());

    HandleChunkMeshGeneration(world);

    GeneratePendingMipmaps(ctx.cmd_buffer);

    GfxCopyPass upload_pass = GfxBeginCopyPass("Upload", ctx.cmd_buffer);
    {
        UploadPendingChunkMeshes(&upload_pass, Max_Chunk_Upload_Per_Frame);
    }
    GfxEndCopyPass(&upload_pass);

    Std140FrameInfo *frame_info = Alloc<Std140FrameInfo>(FrameDataAllocator());
    *frame_info = {
        .window_pixel_size={(float)window_w, (float)window_h},
        .window_scale_factor=1,
        .sun_direction=world->sun_direction,
        .sun_color={1,1,1,2},
        .camera={
            .fov_in_degrees=world->camera.fov_in_degrees,
            .z_near_dist=world->camera.z_near_dist,
            .z_far_dist=world->camera.z_far_dist,
            .position=TranslationVector(world->camera.transform),
            .right=RightVector(world->camera.transform),
            .up=UpVector(world->camera.transform),
            .direction=ForwardVector(world->camera.transform),
            .transform=Transposed(world->camera.transform),
            .view=Transposed(world->camera.view),
            .projection=Transposed(world->camera.projection),
        },
        .texture_atlas_size={Block_Atlas_Size, Block_Atlas_Size},
        .texture_block_size={Block_Texture_Size, Block_Texture_Size},
        .texture_block_border={Block_Texture_Border, Block_Texture_Border},
        .shadow_map={
            .resolution=(int)GetDesc(&g_shadow_map_texture).width,
            .noise_resolution=(int)GetDesc(&g_shadow_map_noise_texture).width,
            .depth_bias_min_max={g_shadow_map_min_depth_bias, g_shadow_map_max_depth_bias},
            .normal_bias=g_shadow_map_normal_bias,
            .filter_radius=g_shadow_map_filter_radius,
            .cascade_matrices={
                Transposed(GetShadowMapCascadeMatrix(world->sun_direction, world->camera.transform, 0)),
                Transposed(GetShadowMapCascadeMatrix(world->sun_direction, world->camera.transform, 1)),
                Transposed(GetShadowMapCascadeMatrix(world->sun_direction, world->camera.transform, 2)),
                Transposed(GetShadowMapCascadeMatrix(world->sun_direction, world->camera.transform, 3)),
            },
            .cascade_sizes={
                {g_shadow_map_cascade_sizes[0],0,0,0},
                {g_shadow_map_cascade_sizes[1],0,0,0},
                {g_shadow_map_cascade_sizes[2],0,0,0},
                {g_shadow_map_cascade_sizes[3],0,0,0},
            },
        },
    };
    Assert(frame_info != null);
    ctx.frame_info_offset = GetBufferOffset(FrameDataGfxAllocator(), frame_info);

    ShadowMapPass(&ctx);

    {
        GfxRenderPassDesc pass_desc{};
        GfxSetColorAttachment(&pass_desc, 0, &g_main_color_texture);
        GfxSetDepthAttachment(&pass_desc, &g_main_depth_texture);
        GfxClearColor(&pass_desc, 0, {0.106, 0.478, 0.82,1});
        GfxClearDepth(&pass_desc, 1);

        static const u64 a = offsetof(Std140FrameInfo, shadow_map.cascade_sizes[1]);

        auto pass = GfxBeginRenderPass("Chunk", ctx.cmd_buffer, pass_desc);
        {
            GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
            GfxSetPipelineState(&pass, &g_chunk_pipeline);

            auto vertex_frame_info = GfxGetVertexStageBinding(&g_chunk_pipeline, "frame_info_buffer");
            auto fragment_frame_info = GfxGetFragmentStageBinding(&g_chunk_pipeline, "frame_info_buffer");
            auto fragment_block_atlas = GfxGetFragmentStageBinding(&g_chunk_pipeline, "block_atlas");
            auto fragment_shadow_map = GfxGetFragmentStageBinding(&g_chunk_pipeline, "shadow_map");
            auto fragment_shadow_map_noise = GfxGetFragmentStageBinding(&g_chunk_pipeline, "shadow_map_noise");

            GfxSetBuffer(&pass, vertex_frame_info, FrameDataBuffer(), ctx.frame_info_offset, sizeof(Std140FrameInfo));
            GfxSetBuffer(&pass, fragment_frame_info, FrameDataBuffer(), ctx.frame_info_offset, sizeof(Std140FrameInfo));

            if (g_show_debug_atlas)
                GfxSetTexture(&pass, fragment_block_atlas, &g_debug_block_face_atlas);
            else
                GfxSetTexture(&pass, fragment_block_atlas, &g_block_atlas);

            GfxSetSamplerState(&pass, fragment_block_atlas, &g_block_sampler);

            GfxSetTexture(&pass, fragment_shadow_map, &g_shadow_map_texture);
            GfxSetSamplerState(&pass, fragment_shadow_map, &g_shadow_map_sampler);

            GfxSetTexture(&pass, fragment_shadow_map_noise, &g_shadow_map_noise_texture);
            GfxSetSamplerState(&pass, fragment_shadow_map_noise, &g_shadow_map_noise_sampler);

            for (int type = 0; type < ChunkMeshType_Count; type += 1)
            {
                foreach (i, world->all_chunks)
                {
                    auto chunk = world->all_chunks[i];
                    if (!chunk->mesh.uploaded)
                        continue;

                    Vec2f chunk_position = Vec2f{(float)chunk->x * Chunk_Size, (float)chunk->z * Chunk_Size};
                    Vec2f camera_position = Vec2f{world->camera.position.x, world->camera.position.z};
                    if (Length(camera_position - chunk_position) > g_settings.render_distance * Chunk_Size)
                        continue;

                    GfxSetVertexBuffer(&pass, Default_Vertex_Buffer_Index, &chunk->mesh.vertex_buffer, 0, sizeof(BlockVertex) * chunk->mesh.vertex_count, sizeof(BlockVertex));

                    u32 vertex_offset = chunk->mesh.mesh_type_vertex_offsets[type];
                    u32 index_offset = chunk->mesh.mesh_type_index_offsets[type];
                    u32 index_count = chunk->mesh.mesh_type_index_counts[type];
                    GfxDrawIndexedPrimitives(&pass, &chunk->mesh.index_buffer, index_count, GfxIndexType_Uint32, 1, vertex_offset, index_offset, (u32)i);
                }
            }
        }
        GfxEndRenderPass(&pass);
    }

    {
        GfxRenderPassDesc pass_desc{};
        GfxSetColorAttachment(&pass_desc, 0, GfxGetSwapchainTexture());

        auto pass = GfxBeginRenderPass("Post Processing", ctx.cmd_buffer, pass_desc);
        {
            GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
            GfxSetPipelineState(&pass, &g_post_processing_pipeline);

            auto fragment_main_texture = GfxGetFragmentStageBinding(&g_post_processing_pipeline, "main_texture");
            GfxSetTexture(&pass, fragment_main_texture, &g_main_color_texture);
            GfxSetSamplerState(&pass, fragment_main_texture, &g_linear_sampler);

            GfxDrawPrimitives(&pass, 6, 1);
        }
        GfxEndRenderPass(&pass);
    }

    UIRenderPass(&ctx);

    GfxExecuteCommandBuffer(ctx.cmd_buffer);

    GfxSubmitFrame();
}
