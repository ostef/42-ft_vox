#include "Core.hpp"
#include "World.hpp"

static bool CompareChunkKeys(ChunkKey a, ChunkKey b)
{
    return a.x == b.x && a.z == b.z;
}

static u64 HashChunkKey(ChunkKey key)
{
    return Fnv1aHash(&key, sizeof(ChunkKey));
}

void InitWorld(World *world, u32 seed)
{
    world->seed = seed;

    world->chunks_by_position.allocator = heap;
    world->chunks_by_position.Compare = CompareChunkKeys;
    world->chunks_by_position.Hash = HashChunkKey;

    world->all_chunks.allocator = heap;
    world->dirty_chunks.allocator = heap;
}

void GenerateChunk(World *world, s16 x, s16 z)
{
    Chunk *chunk = Alloc<Chunk>(heap);
    chunk->x = x;
    chunk->z = z;

    for (int iz = 0; iz < Chunk_Height; iz += 1)
    {
        for (int iy = 0; iy < Chunk_Size; iy += 1)
        {
            for (int ix = 0; ix < Chunk_Size; ix += 1)
            {
                int index = iz * Chunk_Size * Chunk_Size + iy * Chunk_Size + ix;
                chunk->blocks[index] = Block_Stone;
            }
        }
    }

    chunk->is_generated = true;
    HashMapInsert(&world->chunks_by_position, {.x=x, .z=z}, chunk);
    ArrayPush(&world->all_chunks, chunk);
    ArrayPush(&world->dirty_chunks, chunk);
}
