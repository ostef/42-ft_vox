#include "Graphics/Renderer.hpp"
#include "World.hpp"

struct ChunkMeshUpload
{
    Array<BlockVertex> vertices[ChunkMeshType_Count] = {};
    Array<u32> indices[ChunkMeshType_Count] = {};
    Mesh *mesh = null;
};

// Enough for the maximum needed for two chunks
#define Chunk_Mesh_Allocator_Capacity (2 * 4 * 6 * (Chunk_Size * Chunk_Size * Chunk_Height * (sizeof(BlockVertex) + sizeof(u32))))

Array<ChunkMeshUpload> g_pending_chunk_mesh_uploads;
GfxAllocator g_chunk_upload_allocators[Gfx_Max_Frames_In_Flight];

GfxAllocator *CurrentChunkMeshGfxAllocator()
{
    return &g_chunk_upload_allocators[GfxGetBackbufferIndex()];
}

static BlockVertex *PushVertex(Array<BlockVertex> *vertices, Block block, float block_height, BlockFace face, QuadCorner corner)
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
    if (!visible_faces || block == Block_Air)
        return;

    u32 index_start = (u32)vertices->count;
    if (visible_faces & BlockFaceFlag_East)
    {
        auto v = PushVertex(vertices, block, block_height, BlockFace_East, QuadCorner_BottomLeft);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East, QuadCorner_TopRight);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_East, QuadCorner_TopLeft);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_East, QuadCorner_BottomRight);
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
        auto v = PushVertex(vertices, block, block_height, BlockFace_West, QuadCorner_BottomLeft);
        v->position = position + Vec3f{0,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_West, QuadCorner_TopRight);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_West, QuadCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_West, QuadCorner_BottomRight);
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
        auto v = PushVertex(vertices, block, block_height, BlockFace_Top, QuadCorner_BottomLeft);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, QuadCorner_TopRight);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, QuadCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Top, QuadCorner_BottomRight);
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
        auto v = PushVertex(vertices, block, block_height, BlockFace_Bottom, QuadCorner_BottomLeft);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, QuadCorner_TopRight);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, QuadCorner_TopLeft);
        v->position = position + Vec3f{1,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_Bottom, QuadCorner_BottomRight);
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
        auto v = PushVertex(vertices, block, block_height, BlockFace_North, QuadCorner_BottomLeft);
        v->position = position + Vec3f{1,0,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, QuadCorner_TopRight);
        v->position = position + Vec3f{0,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, QuadCorner_TopLeft);
        v->position = position + Vec3f{1,block_height,1};

        v = PushVertex(vertices, block, block_height, BlockFace_North, QuadCorner_BottomRight);
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
        auto v = PushVertex(vertices, block, block_height, BlockFace_South, QuadCorner_BottomLeft);
        v->position = position + Vec3f{0,0,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, QuadCorner_TopRight);
        v->position = position + Vec3f{1,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, QuadCorner_TopLeft);
        v->position = position + Vec3f{0,block_height,0};

        v = PushVertex(vertices, block, block_height, BlockFace_South, QuadCorner_BottomRight);
        v->position = position + Vec3f{1,0,0};

        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }
}

static void AppendChunkMeshUpload(Chunk *chunk, Array<BlockVertex> vertices[ChunkMeshType_Count], Array<u32> indices[ChunkMeshType_Count]);

void GenerateChunkMeshWorker(ThreadGroup *group, void *data)
{
    auto work = (ChunkMeshWork *)data;
    auto chunk = work->chunk;

    for (int i = 0; i < ChunkMeshType_Count; i += 1)
    {
        work->vertices[i].allocator = heap;
        ArrayReserve(&work->vertices[i], chunk->mesh.vertex_count);

        work->indices[i].allocator = heap;
        ArrayReserve(&work->indices[i], chunk->mesh.index_count);
    }

    Vec3f chunk_position = Vec3f{(float)chunk->x * Chunk_Size, 0, (float)chunk->z * Chunk_Size};
    for (int y = 0; y < Chunk_Height; y += 1)
    {
        for (int z = 0; z < Chunk_Size; z += 1)
        {
            for (int x = 0; x < Chunk_Size; x += 1)
            {
                Block block = GetBlock(chunk, x, y, z);
                BlockInfo info = Block_Infos[block];
                if (info.mesh_id < 0)
                    continue;

                float block_height = GetBlockHeight(chunk, block, x, y, z);

                auto east   = GetBlockInNeighbors(chunk, x + 1, y, z);
                auto west   = GetBlockInNeighbors(chunk, x - 1, y, z);
                auto top    = GetBlockInNeighbors(chunk, x, y + 1, z);
                auto bottom = GetBlockInNeighbors(chunk, x, y - 1, z);
                auto north  = GetBlockInNeighbors(chunk, x, y, z + 1);
                auto south  = GetBlockInNeighbors(chunk, x, y, z - 1);

                BlockFaceFlags faces = 0;
                if (Block_Infos[east].mesh_id != info.mesh_id || GetBlockHeight(chunk, east, x + 1, y, z) != block_height)
                    faces |= BlockFaceFlag_East;
                if (Block_Infos[west].mesh_id != info.mesh_id || GetBlockHeight(chunk, west, x - 1, y, z) != block_height)
                    faces |= BlockFaceFlag_West;
                if (Block_Infos[north].mesh_id != info.mesh_id || GetBlockHeight(chunk, north, x, y, z + 1) != block_height)
                    faces |= BlockFaceFlag_North;
                if (Block_Infos[south].mesh_id != info.mesh_id || GetBlockHeight(chunk, south, x, y, z - 1) != block_height)
                    faces |= BlockFaceFlag_South;
                if (Block_Infos[top].mesh_id != info.mesh_id || block_height != 1)
                    faces |= BlockFaceFlag_Top;
                if (Block_Infos[bottom].mesh_id != info.mesh_id || GetBlockHeight(chunk, bottom, x, y - 1, z) != block_height)
                    faces |= BlockFaceFlag_Bottom;

                PushBlockVertices(&work->vertices[info.mesh_id], &work->indices[info.mesh_id], block, chunk_position + Vec3f{(float)x, (float)y, (float)z}, faces, block_height);
            }
        }
    }
}

void HandleChunkMeshGeneration(World *world)
{
    foreach (i, world->dirty_chunks)
    {
        auto chunk = world->dirty_chunks[i];
        if (!chunk->is_generated)
            continue;
        if (chunk->east && !chunk->east->is_generated)
            continue;
        if (chunk->west && !chunk->west->is_generated)
            continue;
        if (chunk->north && !chunk->north->is_generated)
            continue;
        if (chunk->south && !chunk->south->is_generated)
            continue;

        auto work = Alloc<ChunkMeshWork>(heap);
        work->chunk = chunk;
        AddWork(&world->chunk_mesh_generation_thread_group, work);

        ArrayOrderedRemoveAt(&world->dirty_chunks, i);
        i -= 1;
    }

    auto generated_chunk_meshes = GetCompletedWork(&world->chunk_mesh_generation_thread_group);
    foreach (i, generated_chunk_meshes)
    {
        auto work = (ChunkMeshWork *)generated_chunk_meshes[i];

        work->chunk->mesh.vertex_count = 0;
        work->chunk->mesh.index_count = 0;

        s64 total_vertex_count = 0;
        s64 total_index_count = 0;
        for (int j = 0; j < ChunkMeshType_Count; j += 1)
        {
            work->chunk->mesh.vertex_count += work->vertices[j].count;
            work->chunk->mesh.index_count += work->indices[j].count;
        }

        // Recreate buffers if they are too small
        if (work->chunk->mesh.vertex_count * sizeof(BlockVertex) > (u64)GetDesc(&work->chunk->mesh.vertex_buffer).size)
        {
            if (!IsNull(&work->chunk->mesh.vertex_buffer))
                GfxDestroyBuffer(&work->chunk->mesh.vertex_buffer);

            GfxBufferDesc desc{};
            desc.size = work->chunk->mesh.vertex_count * sizeof(BlockVertex);
            desc.usage = GfxBufferUsage_VertexBuffer;
            work->chunk->mesh.vertex_buffer = GfxCreateBuffer(TPrintf("Chunk %d %d Vertices", work->chunk->x, work->chunk->z), desc);
            Assert(!IsNull(&work->chunk->mesh.vertex_buffer));
        }

        if (work->chunk->mesh.index_count * sizeof(u32) > (u64)GetDesc(&work->chunk->mesh.index_buffer).size)
        {
            if (!IsNull(&work->chunk->mesh.index_buffer))
                GfxDestroyBuffer(&work->chunk->mesh.index_buffer);

            GfxBufferDesc desc{};
            desc.size = work->chunk->mesh.index_count * sizeof(u32);
            desc.usage = GfxBufferUsage_IndexBuffer;
            work->chunk->mesh.index_buffer = GfxCreateBuffer(TPrintf("Chunk %d %d Indices", work->chunk->x, work->chunk->z), desc);
            Assert(!IsNull(&work->chunk->mesh.index_buffer));
        }

        AppendChunkMeshUpload(work->chunk, work->vertices, work->indices);

        Free(work, heap);
    }
}

void CancelChunkMeshUpload(Chunk *chunk)
{
    foreach (i, g_pending_chunk_mesh_uploads)
    {
        auto upload = g_pending_chunk_mesh_uploads[i];
        if (upload.mesh == &chunk->mesh)
        {
            for (int j = 0; j < ChunkMeshType_Count; i += 1)
            {
                ArrayFree(&upload.vertices[j]);
                ArrayFree(&upload.indices[j]);
            }

            ArrayOrderedRemoveAt(&g_pending_chunk_mesh_uploads, i);
            break;
        }
    }
}

void AppendChunkMeshUpload(Chunk *chunk, Array<BlockVertex> vertices[ChunkMeshType_Count], Array<u32> indices[ChunkMeshType_Count])
{
    foreach (i, g_pending_chunk_mesh_uploads)
    {
        auto upload = &g_pending_chunk_mesh_uploads[i];
        if (upload->mesh == &chunk->mesh)
        {
            for (int j = 0; j < ChunkMeshType_Count; j += 1)
            {
                ArrayFree(&upload->vertices[j]);
                ArrayFree(&upload->indices[j]);

                upload->vertices[j] = vertices[j];
                upload->indices[j] = indices[j];
            }

            upload->mesh->uploaded = false;

            return;
        }
    }

    ChunkMeshUpload upload{};
    for (int i = 0; i < ChunkMeshType_Count; i += 1)
    {
        upload.vertices[i] = vertices[i];
        upload.indices[i] = indices[i];
    }

    upload.mesh = &chunk->mesh;
    upload.mesh->uploaded = false;

    ArrayPush(&g_pending_chunk_mesh_uploads, upload);
}

void UploadPendingChunkMeshes(GfxCopyPass *pass, int max_uploads)
{
    float time_start = GetTimeInSeconds();

    ResetGfxAllocator(CurrentChunkMeshGfxAllocator());

    if (g_pending_chunk_mesh_uploads.count <= 0)
        return;

    int num_uploaded = 0;
    GfxAllocator *gfx_allocator = CurrentChunkMeshGfxAllocator();
    Allocator allocator = MakeAllocator(gfx_allocator);

    foreach (i, g_pending_chunk_mesh_uploads)
    {
        if (num_uploaded >= max_uploads)
            break;

        auto upload = g_pending_chunk_mesh_uploads[i];

        s64 vertices_size = 0;
        s64 indices_size = 0;
        for (int j = 0; j < ChunkMeshType_Count; j += 1)
        {
            vertices_size += upload.vertices[j].count * sizeof(BlockVertex);
            indices_size += upload.indices[j].count * sizeof(u32);
        }

        s64 total_size = vertices_size + indices_size;

        void *ptr = Alloc(total_size, allocator);
        if (!ptr)
            continue;

        s64 vertices_offset = GetBufferOffset(gfx_allocator, ptr);
        s64 indices_offset = vertices_offset + vertices_size;

        s64 vertices_memcpy_offset = 0;
        for (int j = 0; j < ChunkMeshType_Count; j += 1)
        {
            upload.mesh->mesh_type_vertex_offsets[j] = vertices_memcpy_offset / sizeof(BlockVertex);

            memcpy((u8 *)ptr + vertices_memcpy_offset, upload.vertices[j].data, upload.vertices[j].count * sizeof(BlockVertex));
            vertices_memcpy_offset += upload.vertices[j].count * sizeof(BlockVertex);
        }

        ptr = (u8 *)ptr + vertices_size;
        s64 indices_memcpy_offset = 0;
        for (int j = 0; j < ChunkMeshType_Count; j += 1)
        {
            upload.mesh->mesh_type_index_offsets[j] = indices_memcpy_offset / sizeof(u32);
            upload.mesh->mesh_type_index_counts[j] = upload.indices[j].count;

            memcpy((u8 *)ptr + indices_memcpy_offset, upload.indices[j].data, upload.indices[j].count * sizeof(u32));
            indices_memcpy_offset += upload.indices[j].count * sizeof(u32);
        }

        for (int j = 0; j < ChunkMeshType_Count; j += 1)
        {
            ArrayFree(&upload.vertices[j]);
            ArrayFree(&upload.indices[j]);
        }

        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, vertices_offset, &upload.mesh->vertex_buffer, 0, vertices_size);
        GfxCopyBufferToBuffer(pass, &gfx_allocator->buffer, indices_offset, &upload.mesh->index_buffer, 0, indices_size);

        upload.mesh->uploaded = true;

        ArrayOrderedRemoveAt(&g_pending_chunk_mesh_uploads, i);
        i -= 1;
        num_uploaded += 1;
    }

    FlushGfxAllocator(gfx_allocator);

    float time_end = GetTimeInSeconds();
    LogMessage(Log_Graphics, "Uploaded %d chunks (%lld pending) in %f s", num_uploaded, g_pending_chunk_mesh_uploads.count, time_end - time_start);
}

void InitChunkMeshUploader()
{
    g_pending_chunk_mesh_uploads.allocator = heap;

    for (int i = 0; i < Gfx_Max_Frames_In_Flight; i += 1)
        InitGfxAllocator(&g_chunk_upload_allocators[i], TPrintf("Chunk Mesh Allocator %d", i), Chunk_Mesh_Allocator_Capacity);
}
