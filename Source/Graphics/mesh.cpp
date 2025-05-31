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

union SurroundingBlocks
{
    Block blocks[3 * 3 * 3];
    struct
    {
        Block bottom_south_west;
        Block bottom_south;
        Block bottom_south_east;
        Block bottom_west;
        Block bottom;
        Block bottom_east;
        Block bottom_north_west;
        Block bottom_north;
        Block bottom_north_east;
        Block south_west;
        Block south;
        Block south_east;
        Block west;
        Block center;
        Block east;
        Block north_west;
        Block north;
        Block north_east;
        Block top_south_west;
        Block top_south;
        Block top_south_east;
        Block top_west;
        Block top;
        Block top_east;
        Block top_north_west;
        Block top_north;
        Block top_north_east;
    };
};

static void SetBlock(SurroundingBlocks *blocks, int x, int y, int z, Block block)
{
    if (x < -1 || x > 1)
        return;
    if (y < -1 || y > 1)
        return;
    if (z < -1 || z > 1)
        return;

    blocks->blocks[(y + 1) * 3 * 3 + (z + 1) * 3 + (x + 1)] = block;
}

static Block GetBlock(SurroundingBlocks *blocks, int x, int y, int z)
{
    if (x < -1 || x > 1)
        return Block_Air;
    if (y < -1 || y > 1)
        return Block_Air;
    if (z < -1 || z > 1)
        return Block_Air;

    return blocks->blocks[(y + 1) * 3 * 3 + (z + 1) * 3 + (x + 1)];
}

static float GetBlockHeight(SurroundingBlocks *blocks, int x, int y, int z)
{
    Block block = GetBlock(blocks, x, y, z);
    Block above = GetBlock(blocks, x, y + 1, z);
    if (block == Block_Water && above != Block_Water)
        return 14 / 16.0;

    return 1;
}

static uint GetOcclusionFactor(SurroundingBlocks *blocks, BlockFace face, int face_u, int face_v)
{
    Vec3f normal = Block_Normals[face];
    Vec3f tangent = Block_Tangents[face];
    Vec3f bitangent = Block_Bitangents[face];

    int side_u = face_u * 2 - 1;
    int side_v = face_v * 2 - 1;
    Vec3f p_side0  = normal + tangent * side_u;
    Vec3f p_side1  = normal + bitangent * side_v;
    Vec3f p_corner = normal + tangent * side_u + bitangent * side_v;
    Block side0  = GetBlock(blocks, (int)p_side0.x, (int)p_side0.y, (int)p_side0.z);
    Block side1  = GetBlock(blocks, (int)p_side1.x, (int)p_side1.y, (int)p_side1.z);
    Block corner = GetBlock(blocks, (int)p_corner.x, (int)p_corner.y, (int)p_corner.z);

    if (side0 != Block_Air || side1 != Block_Air || corner != Block_Air)
        return 0;

    // return 3 - (side0 == Block_Air) - (side1 == Block_Air) - (corner == Block_Air);
    return 1;
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

static void PushBlockFace(
    Array<BlockVertex> *vertices, Array<u32> *indices,
    Block block,
    BlockFace face,
    Vec3f block_position,
    float block_height,
    int o00, int o11, int o01, int o10
)
{
    Vec3f p00 = Block_Face_Start[face];
    Vec3f t = Block_Tangents[face];
    Vec3f b = Block_Bitangents[face];

    if (face != BlockFace_Bottom && face != BlockFace_Top)
        b *= block_height;
    else if (face == BlockFace_Top)
        p00.y *= block_height;
    p00 += block_position;

    u32 index_start = (u32)vertices->count;

    auto v = PushVertex(vertices, block, block_height, face, QuadCorner_BottomLeft);
    v->position = p00;
    v->occlusion = o00;

    v = PushVertex(vertices, block, block_height, face, QuadCorner_TopRight);
    v->position = p00 + t + b;
    v->occlusion = o11;

    v = PushVertex(vertices, block, block_height, face, QuadCorner_TopLeft);
    v->position = p00 + b;
    v->occlusion = o01;

    v = PushVertex(vertices, block, block_height, face, QuadCorner_BottomRight);
    v->position = p00 + t;
    v->occlusion = o10;

    if (o00 + o11 == 0 || o01 + o10 == 1)
    {
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
    }
    else
    {
        ArrayPush(indices, index_start + 0);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 2);
        ArrayPush(indices, index_start + 3);
        ArrayPush(indices, index_start + 1);
        ArrayPush(indices, index_start + 2);
    }
}

static void PushBlockVertices(
    Array<BlockVertex> *vertices, Array<u32> *indices,
    Block block,
    Vec3f position,
    SurroundingBlocks *surroundings
)
{
    BlockInfo info = Block_Infos[block];
    if (info.mesh_type == ChunkMeshType_Air)
        return;

    float block_height = GetBlockHeight(surroundings, 0, 0, 0);

    BlockFaceFlags visible_faces = 0;
    if (Block_Infos[surroundings->east].mesh_type != info.mesh_type || GetBlockHeight(surroundings, 1, 0, 0) != block_height)
        visible_faces |= BlockFaceFlag_East;
    if (Block_Infos[surroundings->west].mesh_type != info.mesh_type || GetBlockHeight(surroundings, -1, 0, 0) != block_height)
        visible_faces |= BlockFaceFlag_West;
    if (Block_Infos[surroundings->north].mesh_type != info.mesh_type || GetBlockHeight(surroundings, 0, 0, 1) != block_height)
        visible_faces |= BlockFaceFlag_North;
    if (Block_Infos[surroundings->south].mesh_type != info.mesh_type || GetBlockHeight(surroundings, 0, 0, -1) != block_height)
        visible_faces |= BlockFaceFlag_South;
    if (Block_Infos[surroundings->top].mesh_type != info.mesh_type || block_height != 1)
        visible_faces |= BlockFaceFlag_Top;
    if (Block_Infos[surroundings->bottom].mesh_type != info.mesh_type || GetBlockHeight(surroundings, 0, -1, 0) != 1)
        visible_faces |= BlockFaceFlag_Bottom;

    if (!visible_faces || block == Block_Air)
        return;

    if (visible_faces & BlockFaceFlag_East)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_East, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_East, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_East, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_East, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_East, position, block_height, o00, o11, o01, o10);
    }

    if (visible_faces & BlockFaceFlag_West)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_West, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_West, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_West, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_West, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_West, position, block_height, o00, o11, o01, o10);
    }

    if (visible_faces & BlockFaceFlag_Top)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_Top, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_Top, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_Top, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_Top, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_Top, position, block_height, o00, o11, o01, o10);
    }

    if (visible_faces & BlockFaceFlag_Bottom)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_Bottom, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_Bottom, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_Bottom, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_Bottom, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_Bottom, position, block_height, o00, o11, o01, o10);
    }

    if (visible_faces & BlockFaceFlag_North)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_North, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_North, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_North, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_North, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_North, position, block_height, o00, o11, o01, o10);
    }

    if (visible_faces & BlockFaceFlag_South)
    {
        int o00 = GetOcclusionFactor(surroundings, BlockFace_South, 0, 0);
        int o11 = GetOcclusionFactor(surroundings, BlockFace_South, 1, 1);
        int o01 = GetOcclusionFactor(surroundings, BlockFace_South, 0, 1);
        int o10 = GetOcclusionFactor(surroundings, BlockFace_South, 1, 0);

        PushBlockFace(vertices, indices, block, BlockFace_South, position, block_height, o00, o11, o01, o10);
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
                if (info.mesh_type == ChunkMeshType_Air)
                    continue;

                SurroundingBlocks surroundings;
                for (int yy = -1; yy <= 1; yy += 1)
                {
                    for (int zz = -1; zz <= 1; zz += 1)
                    {
                        for (int xx = -1; xx <= 1; xx += 1)
                        {
                            Block block = GetBlockInNeighbors(chunk, x + xx, y + yy, z + zz);
                            SetBlock(&surroundings, xx, yy, zz, block);
                        }
                    }
                }

                PushBlockVertices(&work->vertices[info.mesh_type], &work->indices[info.mesh_type], block, chunk_position + Vec3f{(float)x, (float)y, (float)z}, &surroundings);
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
