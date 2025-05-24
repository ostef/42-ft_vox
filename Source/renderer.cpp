#include "Renderer.hpp"
#include "World.hpp"

static ShaderFile g_shader_files[] = {
    {.name="mesh_geometry"},
    {.name="post_processing"},
};

static GfxAllocator g_frame_data_allocators[Gfx_Max_Frames_In_Flight];

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

static bool LoadShader(ShaderFile *file)
{
    file->stages = 0;

    if (Gfx_Backend == GfxBackend_OpenGL)
    {
        bool has_vertex = false;
        String vert_filename = TPrintf("Shaders/OpenGL/%.*s.vert.glsl", file->name.length, file->name.data);
        if (FileExists(vert_filename))
        {
            has_vertex = true;

            ShaderPreprocessor pp{};
            if (!InitShaderPreprocessor(&pp, vert_filename))
                return false;

            auto pp_result = PreprocessShader(&pp);
            if (!pp_result.ok)
                return false;

            GfxShader shader = GfxLoadShader(file->name, pp_result.source_code, GfxPipelineStage_Vertex);
            if (IsNull(&shader))
                return false;

            GfxDestroyShader(&file->vertex_shader);
            file->vertex_shader = shader;
            file->stages |= GfxPipelineStageFlag_Vertex;
        }
        else
        {
            GfxDestroyShader(&file->vertex_shader);
        }

        bool has_fragment = false;
        String frag_filename = TPrintf("Shaders/OpenGL/%.*s.frag.glsl", file->name.length, file->name.data);
        if (FileExists(frag_filename))
        {
            has_fragment = true;

            ShaderPreprocessor pp{};
            if (!InitShaderPreprocessor(&pp, frag_filename))
                return false;

            auto pp_result = PreprocessShader(&pp);
            if (!pp_result.ok)
                return false;

            GfxShader shader = GfxLoadShader(file->name, pp_result.source_code, GfxPipelineStage_Fragment);
            if (IsNull(&shader))
                return false;

            GfxDestroyShader(&file->fragment_shader);
            file->fragment_shader = shader;
            file->stages |= GfxPipelineStageFlag_Fragment;
        }
        else
        {
            GfxDestroyShader(&file->fragment_shader);
        }

        if (!has_vertex && !has_fragment)
        {
            LogError(Log_Shaders, "Could not find any shader file for shader '%.*s'", file->name.length, file->name.data);
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

void LoadAllShaders()
{
    bool ok = true;
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        ok &= LoadShader(&g_shader_files[i]);
    }

    if (!ok)
    {
        LogError(Log_Shaders, "There were errors when loading shaders. Exiting.");
        exit(1);
    }
}

GfxShader *GetVertexShader(String name)
{
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        if (g_shader_files[i].name == name)
        {
            if (g_shader_files[i].stages & GfxPipelineStageFlag_Vertex)
                return &g_shader_files[i].vertex_shader;

            Panic("Shader '%.*s' has no vertex stage", name.length, name.data);
            return null;
        }
    }

    Panic("No shader named '%.*s'", name.length, name.data);
    return null;
}

GfxShader *GetFragmentShader(String name)
{
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        if (g_shader_files[i].name == name)
        {
            if (g_shader_files[i].stages & GfxPipelineStageFlag_Fragment)
                return &g_shader_files[i].fragment_shader;

            Panic("Shader '%.*s' has no fragment stage", name.length, name.data);
            return null;
        }
    }

    Panic("No shader named '%.*s'", name.length, name.data);
    return null;
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

static BlockVertex *PushVertex(Array<BlockVertex> *vertices, Block block, float block_height, BlockFace face)
{
    auto v = ArrayPush(vertices);
    v->block = block;
    v->block_height = block_height;
    v->face = face;

    return v;
}

static void PushBlockVertices(
    Array<BlockVertex> *vertices, Array<u32> *indices,
    Block block,
    Vec3f position,
    BlockFaceFlags visible_faces,
    float block_height
)
{
    u32 index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_East)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_East);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_East);
        v->position = position + Vec3f{1,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 3);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_West)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_West);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_West);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_West);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_West);
        v->position = position + Vec3f{0,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_Top)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_Top);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Top);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Top);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Top);
        v->position = position + Vec3f{0,block_height,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_Bottom)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_Bottom);
        v->position = position + Vec3f{0,0,0};
s64 GetBufferOffset(GfxAllocator *allocator, void *ptr);

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom);
        v->position = position + Vec3f{0,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 3);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_North)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_North);
        v->position = position + Vec3f{0,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North);
        v->position = position + Vec3f{0,block_height,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 3);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_South)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_South);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South);
        v->position = position + Vec3f{0,block_height,0};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }
}

static void AppendChunkMeshUpload(Chunk *chunk, Array<BlockVertex> vertices, Array<u32> indices);

void GenerateChunkMesh(Chunk *chunk)
{
    Array<BlockVertex> vertices {};
    vertices.allocator = heap;
    ArrayReserve(&vertices, chunk->mesh.vertex_count);

    Array<u32> indices {};
    indices.allocator = heap;
    ArrayReserve(&vertices, chunk->mesh.index_count);

    Vec3f chunk_position = Vec3f{(float)chunk->x * Chunk_Size, 0, (float)chunk->z * Chunk_Size};
    for (int z = 0; z < Chunk_Height; z += 1)
    {
        for (int y = 0; y < Chunk_Size; y += 1)
        {
            for (int x = 0; x < Chunk_Size; x += 1)
            {
                int index = z * Chunk_Size * Chunk_Size + y * Chunk_Size + x;
                PushBlockVertices(&vertices, &indices, chunk->blocks[index], chunk_position + Vec3f{(float)x, (float)y, (float)z}, BlockFaceFlag_All, 1);
            }
        }
    }

    chunk->mesh.vertex_count = (u32)vertices.count;
    chunk->mesh.index_count = (u32)indices.count;

    // Recreate buffers if they are too small
    if (chunk->mesh.vertex_count * sizeof(BlockVertex) > (u64)GetDesc(&chunk->mesh.vertex_buffer).size)
    {
        if (!IsNull(&chunk->mesh.vertex_buffer))
            GfxDestroyBuffer(&chunk->mesh.vertex_buffer);

        GfxBufferDesc desc{};
        desc.size = chunk->mesh.vertex_count * sizeof(BlockVertex);
        desc.usage = GfxBufferUsage_VertexBuffer;
        chunk->mesh.vertex_buffer = GfxCreateBuffer(TPrintf("Chunk %d %d Vertices", chunk->x, chunk->z), desc);
    }

    if (chunk->mesh.index_count * sizeof(u32) > (u64)GetDesc(&chunk->mesh.index_buffer).size)
    {
        if (!IsNull(&chunk->mesh.index_buffer))
            GfxDestroyBuffer(&chunk->mesh.index_buffer);

        GfxBufferDesc desc{};
        desc.size = chunk->mesh.index_count * sizeof(u32);
        desc.usage = GfxBufferUsage_IndexBuffer;
        chunk->mesh.index_buffer = GfxCreateBuffer(TPrintf("Chunk %d %d Indices", chunk->x, chunk->z), desc);
    }

    AppendChunkMeshUpload(chunk, vertices, indices);
}

struct ChunkMeshUpload
{
    Array<BlockVertex> vertices = {};
    Array<u32> indices = {};
    GfxBuffer *vertex_buffer = null;
    GfxBuffer *index_buffer = null;
};

#define Frame_Data_Allocator_Capacity (1 * 1024 * 1024)
// Enough for the maximum needed for two chunks
#define Chunk_Mesh_Allocator_Capacity (2 * 4 * 6 * (Chunk_Size * Chunk_Size * Chunk_Height * (sizeof(BlockVertex) + sizeof(u32))))

Array<ChunkMeshUpload> g_pending_chunk_mesh_uploads;
GfxAllocator g_chunk_upload_allocators[Gfx_Max_Frames_In_Flight];

GfxAllocator *CurrentChunkMeshGfxAllocator()
{
    return &g_chunk_upload_allocators[GfxGetBackbufferIndex()];
}

void AppendChunkMeshUpload(Chunk *chunk, Array<BlockVertex> vertices, Array<u32> indices)
{
    if (vertices.count <= 0 && indices.count <= 0)
        return;

    ChunkMeshUpload upload{};
    upload.vertices = vertices;
    upload.indices = indices;
    upload.vertex_buffer = &chunk->mesh.vertex_buffer;
    upload.index_buffer = &chunk->mesh.index_buffer;

    ArrayPush(&g_pending_chunk_mesh_uploads, upload);
}

void UploadPendingChunkMeshes(GfxCopyPass *pass)
{
    ResetGfxAllocator(CurrentChunkMeshGfxAllocator());

    if (g_pending_chunk_mesh_uploads.count <= 0)
        return;

    int num_uploaded = 0;
    GfxAllocator *gfx_allocator = CurrentChunkMeshGfxAllocator();
    Allocator allocator = MakeAllocator(gfx_allocator);

    foreach (i, g_pending_chunk_mesh_uploads)
    {
        auto upload = g_pending_chunk_mesh_uploads[i];

        s64 vertices_size = upload.vertices.count * sizeof(BlockVertex);
        s64 indices_size = upload.indices.count * sizeof(u32);
        s64 total_size = vertices_size + indices_size;

        void *ptr = Alloc(total_size, allocator);
        if (!ptr)
            continue;

        s64 vertices_offset = GetBufferOffset(gfx_allocator, ptr);
        s64 indices_offset = vertices_offset + vertices_size;

        memcpy(ptr, upload.vertices.data, vertices_size);
        memcpy((u8 *)ptr + vertices_size, upload.indices.data, indices_size);

        ArrayFree(&upload.vertices);
        ArrayFree(&upload.indices);

        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, vertices_offset, upload.vertex_buffer, 0, vertices_size);
        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, indices_offset, upload.index_buffer, 0, indices_size);

        ArrayOrderedRemoveAt(&g_pending_chunk_mesh_uploads, i);
        num_uploaded += 1;
    }

    FlushGfxAllocator(gfx_allocator);

    LogMessage(Log_Graphics, "Uploaded %d chunks (%ld pending)", num_uploaded, g_pending_chunk_mesh_uploads.count);
}

static GfxPipelineState g_post_processing_pipeline;
static GfxPipelineState g_chunk_pipeline;
static GfxTexture g_main_color_texture;
static GfxTexture g_main_depth_texture;

void InitRenderer()
{
    g_pending_chunk_mesh_uploads.allocator = heap;

    for (int i = 0; i < Gfx_Max_Frames_In_Flight; i += 1)
        InitGfxAllocator(&g_frame_data_allocators[i], TPrintf("Frame Data Allocator %d", i), Frame_Data_Allocator_Capacity);

    for (int i = 0; i < Gfx_Max_Frames_In_Flight; i += 1)
        InitGfxAllocator(&g_chunk_upload_allocators[i], TPrintf("Chunk Mesh Allocator %d", i), Chunk_Mesh_Allocator_Capacity);

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
        Array<GfxVertexInputDesc> vertex_layout = {.allocator=heap};
        ArrayPush(&vertex_layout, {
            .format=GfxVertexFormat_Float3,
            .offset=offsetof(BlockVertex, position),
            .stride=sizeof(BlockVertex),
            .buffer_index=Default_Vertex_Buffer_Index
        });

        pipeline_desc.vertex_layout = MakeSlice(vertex_layout);

        g_chunk_pipeline = GfxCreatePipelineState("Chunk", pipeline_desc);
        Assert(!IsNull(&g_chunk_pipeline));
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

void RenderGraphics(World *world)
{
    int window_w, window_h;
    SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

    RecreateRenderTargets();

    GfxBeginFrame();
    GfxCommandBuffer cmd_buffer = GfxCreateCommandBuffer("Frame");

    ResetGfxAllocator(FrameDataGfxAllocator());

    // Generate dirty chunk meshes
    foreach (i, world->dirty_chunks)
        GenerateChunkMesh(world->dirty_chunks[i]);

    ArrayClear(&world->dirty_chunks);

    GfxCopyPass upload_pass = GfxBeginCopyPass("Upload", &cmd_buffer);
    {
        UploadPendingChunkMeshes(&upload_pass);
    }
    GfxEndCopyPass(&upload_pass);

    Std140FrameInfo *frame_info = Alloc<Std140FrameInfo>(FrameDataAllocator());
    *frame_info = {
        .window_pixel_size={(float)window_w, (float)window_h},
        .window_scale_factor=1,
        .camera={
            .fov_in_degrees=world->camera.fov_in_degrees,
            .z_near_dist=world->camera.z_near_dist,
            .z_far_dist=world->camera.z_far_dist,
            .transform=Transposed(world->camera.transform),
            .view=Transposed(world->camera.view),
            .projection=Transposed(world->camera.projection),
        },
    };
    Assert(frame_info != null);
    s64 frame_info_offset = GetBufferOffset(FrameDataGfxAllocator(), frame_info);

    Std430ChunkInfo *chunk_infos = Alloc<Std430ChunkInfo>(world->all_chunks.count, FrameDataAllocator());
    Assert(chunk_infos != null);
    s64 chunk_offset = GetBufferOffset(FrameDataGfxAllocator(), chunk_infos);

    foreach (i, world->all_chunks)
    {
        auto chunk = world->all_chunks[i];
        chunk_infos[i] = {
            .transform=Transposed(Mat4fTranslate(Vec3f{(float)chunk->x * Chunk_Size, 0, (float)chunk->z * Chunk_Size})),
        };
    }

    {
        GfxRenderPassDesc pass_desc{};
        GfxSetColorAttachment(&pass_desc, 0, &g_main_color_texture);
        GfxSetDepthAttachment(&pass_desc, &g_main_depth_texture);
        GfxClearColor(&pass_desc, 0, {0,0,0,1});
        GfxClearDepth(&pass_desc, 1);

        auto pass = GfxBeginRenderPass("Chunk", &cmd_buffer, pass_desc);
        {
            GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
            GfxSetPipelineState(&pass, &g_chunk_pipeline);

            auto vertex_frame_info = GfxGetVertexStageBinding(&g_chunk_pipeline, "frame_info_buffer");
            auto vertex_chunk_info = GfxGetVertexStageBinding(&g_chunk_pipeline, "chunk_info_buffer");

            GfxSetBuffer(&pass, vertex_frame_info, FrameDataBuffer(), frame_info_offset, sizeof(Std140FrameInfo));
            GfxSetBuffer(&pass, vertex_chunk_info, FrameDataBuffer(), chunk_offset, sizeof(Std430ChunkInfo) * world->all_chunks.count);

            foreach (i, world->all_chunks)
            {
                auto chunk = world->all_chunks[i];

                GfxSetVertexBuffer(&pass, Default_Vertex_Buffer_Index, &chunk->mesh.vertex_buffer, 0, sizeof(BlockVertex) * chunk->mesh.vertex_count, sizeof(BlockVertex));

                GfxDrawIndexedPrimitives(&pass, &chunk->mesh.index_buffer, chunk->mesh.index_count, GfxIndexType_Uint32, 1, 0, 0, (u32)i);
            }
        }
        GfxEndRenderPass(&pass);
    }

    {
        GfxRenderPassDesc pass_desc{};
        GfxSetColorAttachment(&pass_desc, 0, GfxGetSwapchainTexture());
        GfxClearColor(&pass_desc, 0, {0,0,0,1});

        auto pass = GfxBeginRenderPass("Post Processing", &cmd_buffer, pass_desc);
        {
            GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
            GfxSetPipelineState(&pass, &g_post_processing_pipeline);

            auto fragment_main_texture = GfxGetFragmentStageBinding(&g_post_processing_pipeline, "main_texture");
            GfxSetTexture(&pass, fragment_main_texture, &g_main_color_texture);

            GfxDrawPrimitives(&pass, 6, 1);
        }
        GfxEndRenderPass(&pass);

        GfxExecuteCommandBuffer(&cmd_buffer);

        GfxSubmitFrame();
    }
}
