#include "Graphics/Renderer.hpp"
#include "World.hpp"
#include "UI.hpp"

#include <stb_image.h>

#define Max_Chunk_Upload_Per_Frame 50

static GfxAllocator g_frame_data_allocators[Gfx_Max_Frames_In_Flight];

static GfxTexture g_block_atlas;

struct LoadedImage
{
    u32 *pixels = null;
    int width = 0;
    int height = 0;
};

static Result<LoadedImage> LoadImage(String filename)
{
    int width, height;
    u32 *pixels = (u32 *)stbi_load(CloneToCString(filename, temp), &width, &height, null, 4);
    if (!pixels)
        return Result<LoadedImage>::Bad(false);

    LoadedImage result;
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    if (result.width != Block_Texture_Size || result.height != Block_Texture_Size)
    {
        LogError(Log_Graphics, "Block texture '%.*s' has size %dx%d but we expected %dx%d", FSTR(filename), width, height, Block_Texture_Size, Block_Texture_Size);

        return Result<LoadedImage>::Bad(false);
    }

    return Result<LoadedImage>::Good(result, true);
}

static void BlitPixels(u32 *dest, Vec3u dest_size, Vec3u dest_origin, LoadedImage *src_img)
{
    for (int y = 0; y < src_img->height; y += 1)
    {
        for (int x = 0; x < src_img->width; x += 1)
        {
            int index = dest_origin.z * dest_size.x * dest_size.y + (dest_origin.y + y) * dest_size.x + (dest_origin.x + x);
            dest[index] = src_img->pixels[y * src_img->width + x];
        }
    }
}

static void LoadBlockAtlasTexture()
{
    u32 *pixels = Alloc<u32>(Block_Atlas_Size * Block_Atlas_Size * 6, heap);
    memset(pixels, 0, Block_Atlas_Size * Block_Atlas_Size * 6 * sizeof(u32));

    for (int i = 1; i < Block_Count; i += 1)
    {
        String name = Block_Infos[i].name;
        String filename = TPrintf("Data/Blocks/%.*s.png", FSTR(name));
        String filename_top = TPrintf("Data/Blocks/%.*s_top.png", FSTR(name));
        String filename_bottom = TPrintf("Data/Blocks/%.*s_bottom.png", FSTR(name));
        String filename_east = TPrintf("Data/Blocks/%.*s_east.png", FSTR(name));
        String filename_west = TPrintf("Data/Blocks/%.*s_west.png", FSTR(name));
        String filename_north = TPrintf("Data/Blocks/%.*s_north.png", FSTR(name));
        String filename_south = TPrintf("Data/Blocks/%.*s_south.png", FSTR(name));

        auto image = LoadImage(filename);
        auto image_top = LoadImage(filename_top);
        auto image_bottom = LoadImage(filename_bottom);
        auto image_east = LoadImage(filename_east);
        auto image_west = LoadImage(filename_west);
        auto image_north = LoadImage(filename_north);
        auto image_south = LoadImage(filename_south);
        if (!image_top.ok || !image_bottom.ok || !image_east.ok || !image_west.ok || !image_north.ok || !image_south.ok)
        {
            Assert(image.ok, "Could not load base image for block '%.*s'", FSTR(name));
        }

        uint block_x = ((i - 1) % Block_Atlas_Num_Blocks) * Block_Texture_Size;
        uint block_y = ((i - 1) / Block_Atlas_Num_Blocks) * Block_Texture_Size;
        if (image_east.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_East}, &image_east.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_East}, &image.value);
        if (image_west.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_West}, &image_west.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_West}, &image.value);
        if (image_top.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Top}, &image_top.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Top}, &image.value);
        if (image_bottom.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Bottom}, &image_bottom.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Bottom}, &image.value);
        if (image_north.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_North}, &image_north.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_North}, &image.value);
        if (image_south.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_South}, &image_south.value);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_South}, &image.value);
    }

    GfxTextureDesc desc{};
    desc.type = GfxTextureType_Texture2DArray;
    desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
    desc.width = Block_Atlas_Size;
    desc.height = Block_Atlas_Size;
    desc.array_length = 6;
    desc.usage = GfxTextureUsage_ShaderRead;
    g_block_atlas = GfxCreateTexture("Block Atlas", desc);
    Assert(!IsNull(&g_block_atlas));

    GfxReplaceTextureRegion(&g_block_atlas, {0,0,0}, {Block_Atlas_Size, Block_Atlas_Size, 6}, 0, 0, pixels);
}

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

    return MakeSlice(vertex_layout);
}

void InitRenderer()
{
    LoadBlockAtlasTexture();

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
        pipeline_desc.depth_format = GfxPixelFormat_DepthFloat32;
        pipeline_desc.depth_state = {.enabled=true, .write_enabled=true};
        pipeline_desc.vertex_layout = MakeBlockVertexLayout();

        g_chunk_pipeline = GfxCreatePipelineState("Chunk", pipeline_desc);
        Assert(!IsNull(&g_chunk_pipeline));
    }

    {
        GfxSamplerStateDesc desc{};
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

            GfxSetTexture(&pass, fragment_block_atlas, &g_block_atlas);
            GfxSetSamplerState(&pass, fragment_block_atlas, &g_block_sampler);

            GfxSetTexture(&pass, fragment_shadow_map, &g_shadow_map_texture);
            GfxSetSamplerState(&pass, fragment_shadow_map, &g_shadow_map_sampler);

            GfxSetTexture(&pass, fragment_shadow_map_noise, &g_shadow_map_noise_texture);
            GfxSetSamplerState(&pass, fragment_shadow_map_noise, &g_shadow_map_noise_sampler);

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

                GfxDrawIndexedPrimitives(&pass, &chunk->mesh.index_buffer, chunk->mesh.index_count, GfxIndexType_Uint32, 1, 0, 0, (u32)i);
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

            GfxDrawPrimitives(&pass, 6, 1);
        }
        GfxEndRenderPass(&pass);
    }

    UIRenderPass(&ctx);

    GfxExecuteCommandBuffer(ctx.cmd_buffer);

    GfxSubmitFrame();
}
