#include "Renderer.hpp"
#include "World.hpp"
#include "UI.hpp"

#include <stb_image.h>

static ShaderFile g_shader_files[] = {
    {.name="mesh_geometry"},
    {.name="post_processing"},
    {.name="ui"},
};

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
        LogError(Log_Graphics, "Block texture '%.*s' has size %dx%d but we expected %dx%d", filename.length, filename.data, width, height, Block_Texture_Size, Block_Texture_Size);

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
        String name = Block_Names[i];
        String filename = TPrintf("Data/Blocks/%.*s.png", name.length, name.data);
        String filename_top = TPrintf("Data/Blocks/%.*s_top.png", name.length, name.data);
        String filename_bottom = TPrintf("Data/Blocks/%.*s_bottom.png", name.length, name.data);
        String filename_east = TPrintf("Data/Blocks/%.*s_east.png", name.length, name.data);
        String filename_west = TPrintf("Data/Blocks/%.*s_west.png", name.length, name.data);
        String filename_north = TPrintf("Data/Blocks/%.*s_north.png", name.length, name.data);
        String filename_south = TPrintf("Data/Blocks/%.*s_south.png", name.length, name.data);

        auto image = LoadImage(filename);
        auto image_top = LoadImage(filename_top);
        auto image_bottom = LoadImage(filename_bottom);
        auto image_east = LoadImage(filename_east);
        auto image_west = LoadImage(filename_west);
        auto image_north = LoadImage(filename_north);
        auto image_south = LoadImage(filename_south);
        if (!image_top.ok || !image_bottom.ok || !image_east.ok || !image_west.ok || !image_north.ok || !image_south.ok)
        {
            Assert(image.ok, "Could not load base image for block '%.*s'", name.length, name.data);
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

static BlockVertex *PushVertex(Array<BlockVertex> *vertices, Block block, float block_height, BlockFace face, BlockCorner corner)
{
    auto v = ArrayPush(vertices);
    v->block = block;
    v->block_height = block_height;
    v->block_face = face;
    v->block_corner = corner;

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
        auto v = PushVertex(vertices, block, block_height, BlockFace_East, BlockCorner_BottomLeft);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East, BlockCorner_TopRight);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_East, BlockCorner_TopLeft);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East, BlockCorner_BottomRight);
        v->position = position + Vec3f{1,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_West)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_West, BlockCorner_BottomLeft);
        v->position = position + Vec3f{0,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_West, BlockCorner_TopRight);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_West, BlockCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_West, BlockCorner_BottomRight);
        v->position = position + Vec3f{0,0,0};

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
        auto v = PushVertex(vertices, block, block_height, BlockFace_Top, BlockCorner_BottomLeft);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, BlockCorner_TopRight);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, BlockCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, BlockCorner_BottomRight);
        v->position = position + Vec3f{1,block_height,0};

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
        auto v = PushVertex(vertices, block, block_height, BlockFace_Bottom, BlockCorner_BottomLeft);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, BlockCorner_TopRight);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, BlockCorner_TopLeft);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, BlockCorner_BottomRight);
        v->position = position + Vec3f{0,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_North)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_North, BlockCorner_BottomLeft);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, BlockCorner_TopRight);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, BlockCorner_TopLeft);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, BlockCorner_BottomRight);
        v->position = position + Vec3f{0,0,1};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }

    index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_South)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_South, BlockCorner_BottomLeft);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, BlockCorner_TopRight);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, BlockCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, BlockCorner_BottomRight);
        v->position = position + Vec3f{1,0,0};

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
    for (int y = 0; y < Chunk_Height; y += 1)
    {
        for (int z = 0; z < Chunk_Size; z += 1)
        {
            for (int x = 0; x < Chunk_Size; x += 1)
            {
                Block block = GetBlock(chunk, x, y, z);
                if (block == Block_Air)
                    continue;

                auto east   = GetBlockInNeighbors(chunk, x + 1, y, z);
                auto west   = GetBlockInNeighbors(chunk, x - 1, y, z);
                auto top    = GetBlock(chunk, x, y + 1, z);
                auto bottom = GetBlock(chunk, x, y - 1, z);
                auto north  = GetBlockInNeighbors(chunk, x, y, z + 1);
                auto south  = GetBlockInNeighbors(chunk, x, y, z - 1);

                BlockFaceFlags faces = 0;
                if (east == Block_Air)
                    faces |= BlockFaceFlag_East;
                if (west == Block_Air)
                    faces |= BlockFaceFlag_West;
                if (top == Block_Air)
                    faces |= BlockFaceFlag_Top;
                if (bottom == Block_Air)
                    faces |= BlockFaceFlag_Bottom;
                if (north == Block_Air)
                    faces |= BlockFaceFlag_North;
                if (south == Block_Air)
                    faces |= BlockFaceFlag_South;

                PushBlockVertices(&vertices, &indices, block, chunk_position + Vec3f{(float)x, (float)y, (float)z}, faces, 1);
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
    Mesh *mesh = null;
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

void CancelChunkMeshUpload(Chunk *chunk)
{
    foreach (i, g_pending_chunk_mesh_uploads)
    {
        auto upload = g_pending_chunk_mesh_uploads[i];
        if (upload.mesh == &chunk->mesh)
        {
            ArrayFree(&upload.vertices);
            ArrayFree(&upload.indices);
            ArrayOrderedRemoveAt(&g_pending_chunk_mesh_uploads, i);
            break;
        }
    }
}

void AppendChunkMeshUpload(Chunk *chunk, Array<BlockVertex> vertices, Array<u32> indices)
{
    if (vertices.count <= 0 && indices.count <= 0)
        return;

    ChunkMeshUpload upload{};
    upload.vertices = vertices;
    upload.indices = indices;
    upload.mesh = &chunk->mesh;
    upload.mesh->uploaded = false;

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

        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, vertices_offset, &upload.mesh->vertex_buffer, 0, vertices_size);
        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, indices_offset, &upload.mesh->index_buffer, 0, indices_size);

        upload.mesh->uploaded = true;

        ArrayOrderedRemoveAt(&g_pending_chunk_mesh_uploads, i);
        num_uploaded += 1;
    }

    FlushGfxAllocator(gfx_allocator);

    LogMessage(Log_Graphics, "Uploaded %d chunks (%lld pending)", num_uploaded, g_pending_chunk_mesh_uploads.count);
}

static GfxPipelineState g_post_processing_pipeline;
static GfxPipelineState g_chunk_pipeline;
static GfxTexture g_main_color_texture;
static GfxTexture g_main_depth_texture;
static GfxSamplerState g_block_sampler;

void InitRenderer()
{
    LoadBlockAtlasTexture();

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

        pipeline_desc.vertex_layout = MakeSlice(vertex_layout);

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

void RenderGraphics(World *world)
{
    FrameRenderContext ctx{};

    int window_w, window_h;
    SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

    RecreateRenderTargets();

    GfxBeginFrame();
    GfxCommandBuffer cmd_buffer = GfxCreateCommandBuffer("Frame");
    ctx.cmd_buffer = &cmd_buffer;

    ResetGfxAllocator(FrameDataGfxAllocator());

    // Generate dirty chunk meshes
    foreach (i, world->dirty_chunks)
        GenerateChunkMesh(world->dirty_chunks[i]);

    ArrayClear(&world->dirty_chunks);

    GfxCopyPass upload_pass = GfxBeginCopyPass("Upload", ctx.cmd_buffer);
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
        .texture_atlas_size={Block_Atlas_Size, Block_Atlas_Size},
        .texture_block_size={Block_Texture_Size, Block_Texture_Size},
    };
    Assert(frame_info != null);
    ctx.frame_info_offset = GetBufferOffset(FrameDataGfxAllocator(), frame_info);

    {
        GfxRenderPassDesc pass_desc{};
        GfxSetColorAttachment(&pass_desc, 0, &g_main_color_texture);
        GfxSetDepthAttachment(&pass_desc, &g_main_depth_texture);
        GfxClearColor(&pass_desc, 0, {0.1,0.1,0.1,1});
        GfxClearDepth(&pass_desc, 1);

        auto pass = GfxBeginRenderPass("Chunk", ctx.cmd_buffer, pass_desc);
        {
            GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
            GfxSetPipelineState(&pass, &g_chunk_pipeline);

            auto vertex_frame_info = GfxGetVertexStageBinding(&g_chunk_pipeline, "frame_info_buffer");
            auto fragment_block_atlas = GfxGetFragmentStageBinding(&g_chunk_pipeline, "block_atlas");

            GfxSetBuffer(&pass, vertex_frame_info, FrameDataBuffer(), ctx.frame_info_offset, sizeof(Std140FrameInfo));
            GfxSetTexture(&pass, fragment_block_atlas, &g_block_atlas);
            GfxSetSamplerState(&pass, fragment_block_atlas, &g_block_sampler);

            foreach (i, world->all_chunks)
            {
                auto chunk = world->all_chunks[i];
                if (!chunk->mesh.uploaded)
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

    UIRender(&ctx);

    GfxExecuteCommandBuffer(ctx.cmd_buffer);

    GfxSubmitFrame();
}
