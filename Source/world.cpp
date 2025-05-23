#include "Core.hpp"
#include "World.hpp"
#include "Input.hpp"

static bool CompareChunkKeys(ChunkKey a, ChunkKey b)
{
    return a.x == b.x && a.z == b.z;
}

static u64 HashChunkKey(ChunkKey key)
{
    return Fnv1aHash(&key, sizeof(ChunkKey));
}

void UpdateCamera(Camera *camera)
{
    bool moving = IsMouseButtonDown(MouseButton_Right);
    SDL_SetRelativeMouseMode((SDL_bool)moving);

    if (moving)
    {
        Vec2f rotation = GetMouseDelta() * camera->rotation_speed;

        camera->target_yaw += ToRads(rotation.x);
        camera->target_pitch += ToRads(rotation.y);
        camera->target_pitch = Clamp(camera->target_pitch, ToRads(-90), ToRads(90));

        camera->current_yaw = Lerp(camera->current_yaw, camera->target_yaw, camera->rotation_smoothing);
        camera->current_pitch = Lerp(camera->current_pitch, camera->target_pitch, camera->rotation_smoothing);

        Mat4f rotation_mat = Mat4fRotate({0,1,0}, camera->current_yaw) * Mat4fRotate({1,0,0}, camera->current_pitch);

        Vec3f movement{};
        movement.x = (float)IsKeyDown(SDL_SCANCODE_D) - (float)IsKeyDown(SDL_SCANCODE_A);
        movement.y = (float)(IsKeyDown(SDL_SCANCODE_E) || IsKeyDown(SDL_SCANCODE_SPACE)) - (float)(IsKeyDown(SDL_SCANCODE_Q) || IsKeyDown(SDL_SCANCODE_LCTRL));
        movement.z = (float)IsKeyDown(SDL_SCANCODE_W) - (float)IsKeyDown(SDL_SCANCODE_S);
        movement = Normalized(movement);
        if (IsKeyDown(SDL_SCANCODE_LSHIFT))
            movement *= camera->fast_speed * camera->speed_mult;
        else
            movement *= camera->base_speed * camera->speed_mult;

        camera->position += RightVector(rotation_mat) * movement.x + UpVector(rotation_mat) * movement.y + ForwardVector(rotation_mat) * movement.z;
    }

    // Calculate camera matrices
    int width, height;
    SDL_GetWindowSizeInPixels(g_window, &width, &height);
    float aspect = width / (float)height;

    Mat4f rotation = Mat4fRotate({0,1,0}, camera->current_yaw) * Mat4fRotate({1,0,0}, camera->current_pitch);
    camera->transform = Mat4fTranslate(camera->position) * rotation;
    camera->view = Inverted(camera->transform);
    camera->projection = Mat4fPerspectiveProjection(camera->fov_in_degrees, aspect, camera->z_near_dist, camera->z_far_dist);
}

Block GetBlock(Chunk *chunk, int x, int y, int z)
{
    if (x < 0 || x >= Chunk_Size)
        return Block_Air;
    if (z < 0 || z >= Chunk_Size)
        return Block_Air;
    if (y < 0 || y >= Chunk_Height)
        return Block_Air;

    int index = y * Chunk_Size * Chunk_Size + z * Chunk_Size + x;
    return chunk->blocks[index];
}

Block GetBlockInNeighbors(Chunk *chunk, int x, int y, int z)
{
    if (x < 0)
        return chunk->west ? GetBlockInNeighbors(chunk->west, Chunk_Size - x, y, z) : Block_Air;
    else if (x >= Chunk_Size)
        return chunk->east ? GetBlockInNeighbors(chunk->east, x - Chunk_Size, y, z) : Block_Air;
    if (z < 0)
        return chunk->south ? GetBlockInNeighbors(chunk->south, x, y, Chunk_Size - z) : Block_Air;
    else if (z >= Chunk_Size)
        return chunk->north ? GetBlockInNeighbors(chunk->north, x, y, z - Chunk_Size) : Block_Air;

    return GetBlock(chunk, x, y, z);
}

void InitWorld(World *world, u32 seed)
{
    *world = {};

    world->seed = seed;

    world->camera.position.y = 140;

    world->chunks_by_position.allocator = heap;
    world->chunks_by_position.Compare = CompareChunkKeys;
    world->chunks_by_position.Hash = HashChunkKey;

    world->all_chunks.allocator = heap;
    world->dirty_chunks.allocator = heap;

    world->continentalness_params = {
        .scale=10,
        .octaves=5,
        .persistance=0.7,
    };
    world->continentalness_params.max_amplitude = PerlinFractalMax(world->continentalness_params.octaves, world->continentalness_params.persistance);

    world->erosion_params = {
        .scale=10,
        .octaves=5,
        .persistance=0.7,
    };
    world->erosion_params.max_amplitude = PerlinFractalMax(world->erosion_params.octaves, world->erosion_params.persistance);

    world->peaks_and_valleys_params = {
        .scale=10,
        .octaves=5,
        .persistance=0.7,
    };
    world->peaks_and_valleys_params.max_amplitude = PerlinFractalMax(world->peaks_and_valleys_params.octaves, world->peaks_and_valleys_params.persistance);

    world->squashing_factor_params = {
        .scale=10,
        .octaves=5,
        .persistance=0.7,
    };
    world->squashing_factor_params.max_amplitude = PerlinFractalMax(world->squashing_factor_params.octaves, world->squashing_factor_params.persistance);

    world->level_height_params = {
        .scale=1.2347,
        .octaves=5,
        .persistance=0.7,
    };
    world->level_height_params.max_amplitude = PerlinFractalMax(world->level_height_params.octaves, world->level_height_params.persistance);

    world->squashing_factor_offsets = AllocSlice<Vec2f>(world->squashing_factor_params.octaves, heap);
    world->level_height_offsets = AllocSlice<Vec2f>(world->level_height_params.octaves, heap);

    RNG rng{};
    RandomSeed(&rng, seed * 123456);

    PerlinGenerateOffsets(&rng, &world->squashing_factor_offsets);
    PerlinGenerateOffsets(&rng, &world->level_height_offsets);
}

void DestroyWorld(World *world)
{
    while (world->all_chunks.count > 0)
        DestroyChunk(world, world->all_chunks[0]);

    HashMapFree(&world->chunks_by_position);
    ArrayFree(&world->all_chunks);
    ArrayFree(&world->dirty_chunks);
    Free(world->squashing_factor_offsets.data, heap);
    Free(world->level_height_offsets.data, heap);
}

void DestroyChunk(World *world, Chunk *chunk)
{
    CancelChunkMeshUpload(chunk);

    if (chunk->east)
    {
        chunk->east->west = null;
        MarkChunkDirty(world, chunk->east);
    }
    if (chunk->west)
    {
        chunk->west->east = null;
        MarkChunkDirty(world, chunk->west);
    }
    if (chunk->north)
    {
        chunk->north->south = null;
        MarkChunkDirty(world, chunk->north);
    }
    if (chunk->south)
    {
        chunk->south->north = null;
        MarkChunkDirty(world, chunk->south);
    }

    GfxDestroyBuffer(&chunk->mesh.vertex_buffer);
    GfxDestroyBuffer(&chunk->mesh.index_buffer);

    foreach (i, world->dirty_chunks)
    {
        if (world->dirty_chunks[i] == chunk)
        {
            ArrayOrderedRemoveAt(&world->dirty_chunks, i);
            break;
        }
    }

    foreach (i, world->all_chunks)
    {
        if (world->all_chunks[i] == chunk)
        {
            ArrayOrderedRemoveAt(&world->all_chunks, i);
            break;
        }
    }

    HashMapRemove(&world->chunks_by_position, ChunkKey{chunk->x, chunk->z});

    Free(chunk, heap);
}

float base_height = 127;
float squashing_factor = 1.0;

void GenerateChunk(World *world, s16 x, s16 z)
{
    Chunk *chunk = Alloc<Chunk>(heap);
    chunk->x = x;
    chunk->z = z;

    for (int iy = 0; iy < Chunk_Height; iy += 1)
    {
        for (int iz = 0; iz < Chunk_Size; iz += 1)
        {
            for (int ix = 0; ix < Chunk_Size; ix += 1)
            {
                // auto params = SampleWorldParams(world, (float)(x * Chunk_Size + ix), (float)iy, (float)(z * Chunk_Size + iz));

                float scale = 0.0623;

                float perlin_x = (x * Chunk_Size + ix) * scale;
                float perlin_y = iy * scale;
                float perlin_z = (z * Chunk_Size + iz) * scale;

                float density = PerlinNoise(perlin_x, perlin_y, perlin_z);
                float density_bias = (base_height - iy) * squashing_factor * 0.15;

                int index = iy * Chunk_Size * Chunk_Size + iz * Chunk_Size + ix;
                if (density + density_bias > 0)
                    chunk->blocks[index] = Block_Stone;
                else
                    chunk->blocks[index] = Block_Air;
            }
        }
    }

    chunk->is_generated = true;
    HashMapInsert(&world->chunks_by_position, {.x=x, .z=z}, chunk);
    ArrayPush(&world->all_chunks, chunk);
    ArrayPush(&world->dirty_chunks, chunk);

    chunk->east  = HashMapFind(&world->chunks_by_position, ChunkKey{.x=(s16)(x+1), .z=z});
    if (chunk->east)
    {
        chunk->east->west = chunk;
        MarkChunkDirty(world, chunk->east);
    }

    chunk->west  = HashMapFind(&world->chunks_by_position, ChunkKey{.x=(s16)(x-1), .z=z});
    if (chunk->west)
    {
        chunk->west->east = chunk;
        MarkChunkDirty(world, chunk->west);
    }

    chunk->north = HashMapFind(&world->chunks_by_position, ChunkKey{.x=x, .z=(s16)(z+1)});
    if (chunk->north)
    {
        chunk->north->south = chunk;
        MarkChunkDirty(world, chunk->north);
    }

    chunk->south = HashMapFind(&world->chunks_by_position, ChunkKey{.x=x, .z=(s16)(z-1)});
    if (chunk->south)
    {
        chunk->south->north = chunk;
        MarkChunkDirty(world, chunk->south);
    }
}

void MarkChunkDirty(World *world, Chunk *chunk)
{
    foreach (i, world->dirty_chunks)
    {
        if (world->dirty_chunks[i] == chunk)
            return;
    }

    ArrayPush(&world->dirty_chunks, chunk);
}

WorldParams SampleWorldParams(World *world, float x, float y, float z)
{
    WorldParams result = {
        .squashing_factor = PerlinFractalNoise(world->squashing_factor_params, world->squashing_factor_offsets, x, z),
        .level_height = PerlinFractalNoise(world->level_height_params, world->level_height_offsets, x, z),
    };

    return result;
}

Block GetBlock(World *world, float x, float y, float z);
Block GetBlock(World *world, WorldParams params);
