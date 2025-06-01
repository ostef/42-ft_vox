#include "Core.hpp"
#include "World.hpp"
#include "Input.hpp"
#include "UI.hpp"

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

    UISetMouse(!moving);

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
        return chunk->west ? GetBlock(chunk->west, Chunk_Size + x, y, z) : Block_Air;
    if (x >= Chunk_Size)
        return chunk->east ? GetBlock(chunk->east, x - Chunk_Size, y, z) : Block_Air;
    if (z < 0)
        return chunk->south ? GetBlock(chunk->south, x, y, Chunk_Size + z) : Block_Air;
    if (z >= Chunk_Size)
        return chunk->north ? GetBlock(chunk->north, x, y, z - Chunk_Size) : Block_Air;

    return GetBlock(chunk, x, y, z);
}

float GetBlockHeight(Chunk *chunk, Block block, int x, int y, int z)
{
    Block above = GetBlockInNeighbors(chunk, x, y + 1, z);
    if (block == Block_Water && above != Block_Water)
        return 14 / 16.0;

    return 1;
}

void SetDefaultNoiseParams(World *world)
{
    world->density_params = {};
    world->density_params.scale = 0.06;
    world->density_params.octaves = 3;

    world->continentalness_params = {};
    world->continentalness_params.scale = 0.00113;
    world->continentalness_params.octaves = 6;
    world->continentalness_params.persistance = 0.196;
    world->continentalness_params.lacunarity = 4.085;
    world->continentalness_params.final_amplitude = 2.13;

    world->erosion_params = {};
    world->erosion_params.scale = 0.00096;
    world->erosion_params.octaves = 5;
    world->erosion_params.persistance = 0.168;
    world->erosion_params.lacunarity = 3.897;
    world->erosion_params.final_amplitude = 2.52;

    world->peaks_and_valleys_params = {};
    world->peaks_and_valleys_params.scale = 0.00106;
    world->peaks_and_valleys_params.octaves = 4;
    world->peaks_and_valleys_params.persistance = 0.267;
    world->peaks_and_valleys_params.lacunarity = 4.824;
    world->peaks_and_valleys_params.final_amplitude = 2.97;

    world->continentalness_spline = {};
    AddPoint(&world->continentalness_spline, -1.000, 136.000, 0.000);
    AddPoint(&world->continentalness_spline, -0.200, 136.000, 0.000);
    AddPoint(&world->continentalness_spline, 0.100, 176.000, 0.000);
    AddPoint(&world->continentalness_spline, 0.400, 176.000, 0.000);
    AddPoint(&world->continentalness_spline, 0.500, 206.000, 0.000);
    AddPoint(&world->continentalness_spline, 0.700, 216.000, 0.000);
    AddPoint(&world->continentalness_spline, 1.000, 226.000, 0.000);

    world->erosion_spline = {};
    AddPoint(&world->erosion_spline, 0.000, 1.000, -4.000);
    AddPoint(&world->erosion_spline, 0.400, 0.500, -1.000);
    AddPoint(&world->erosion_spline, 0.690, 0.090, 0.000);
    AddPoint(&world->erosion_spline, 0.800, 0.270, 0.000);
    AddPoint(&world->erosion_spline, 0.900, 0.250, -3.000);
    AddPoint(&world->erosion_spline, 0.980, 0.350, -3.000);
}

static void GenerateChunkWorker(ThreadGroup *group, void *work);

void InitWorld(World *world, u32 seed)
{
    RNG rng{};
    RandomSeed(&rng, seed);

    world->seed = seed;

    world->camera.position.y = Water_Level + 5;

    world->chunks_by_position.allocator = heap;
    world->chunks_by_position.Compare = CompareChunkKeys;
    world->chunks_by_position.Hash = HashChunkKey;
    world->num_generated_chunks = 0;

    world->all_chunks.allocator = heap;
    world->dirty_chunks.allocator = heap;

    InitThreadGroup(&world->chunk_generation_thread_group, "Chunk Generation", GenerateChunkWorker, 20);
    Start(&world->chunk_generation_thread_group);

    InitThreadGroup(&world->chunk_mesh_generation_thread_group, "Chunk Mesh Generation", GenerateChunkMeshWorker, 20);
    Start(&world->chunk_mesh_generation_thread_group);

    world->density_params.max_amplitude = PerlinFractalMax(world->density_params.octaves, world->density_params.persistance);

    world->density_offsets = AllocSlice<Vec3f>(world->density_params.octaves, heap);
    PerlinGenerateOffsets(&rng, &world->density_offsets);

    world->continentalness_params.max_amplitude = PerlinFractalMax(world->continentalness_params.octaves, world->continentalness_params.persistance);

    world->continentalness_offsets = AllocSlice<Vec2f>(world->continentalness_params.octaves, heap);
    PerlinGenerateOffsets(&rng, &world->continentalness_offsets);

    world->erosion_params.max_amplitude = PerlinFractalMax(world->erosion_params.octaves, world->erosion_params.persistance);

    world->erosion_offsets = AllocSlice<Vec2f>(world->erosion_params.octaves, heap);
    PerlinGenerateOffsets(&rng, &world->erosion_offsets);

    world->peaks_and_valleys_params.max_amplitude = PerlinFractalMax(world->peaks_and_valleys_params.octaves, world->peaks_and_valleys_params.persistance);

    world->peaks_and_valleys_offsets = AllocSlice<Vec2f>(world->peaks_and_valleys_params.octaves, heap);
    PerlinGenerateOffsets(&rng, &world->peaks_and_valleys_offsets);
}

void DestroyWorld(World *world)
{
    Stop(&world->chunk_generation_thread_group);
    Stop(&world->chunk_mesh_generation_thread_group);

    while (world->all_chunks.count > 0)
        DestroyChunk(world, world->all_chunks[0]);

    HashMapFree(&world->chunks_by_position);
    ArrayFree(&world->all_chunks);
    ArrayFree(&world->dirty_chunks);

    DestroyThreadGroup(&world->chunk_generation_thread_group);
    DestroyThreadGroup(&world->chunk_mesh_generation_thread_group);
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

void GenerateChunksAroundPoint(World *world, Vec3f point, float radius)
{
    int chunk_min_x = (int)((point.x - radius) / Chunk_Size);
    int chunk_min_z = (int)((point.z - radius) / Chunk_Size);
    int chunk_max_x = (int)((point.x + radius) / Chunk_Size);
    int chunk_max_z = (int)((point.z + radius) / Chunk_Size);
    for (int z = chunk_min_z; z < chunk_max_z; z += 1)
    {
        for (int x = chunk_min_x; x < chunk_max_x; x += 1)
        {
            QueueChunkGeneration(world, x, z);
        }
    }
}

float squashing_factor = 1.0;

struct ChunkGenerationWork
{
    World *world = null;
    Chunk *chunk = null;
};

void QueueChunkGeneration(World *world, s16 x, s16 z)
{
    bool exists = false;
    Chunk **chunk_ptr = HashMapFindOrAdd(&world->chunks_by_position, {.x=x, .z=z}, &exists);
    if (exists)
        return;

    Chunk *chunk = Alloc<Chunk>(heap);
    chunk->x = x;
    chunk->z = z;

    ArrayPush(&world->all_chunks, chunk);
    *chunk_ptr = chunk;

    chunk->east  = HashMapFind(&world->chunks_by_position, ChunkKey{.x=(s16)(chunk->x+1), .z=chunk->z});
    if (chunk->east)
    {
        chunk->east->west = chunk;
        MarkChunkDirty(world, chunk->east);
    }

    chunk->west  = HashMapFind(&world->chunks_by_position, ChunkKey{.x=(s16)(chunk->x-1), .z=chunk->z});
    if (chunk->west)
    {
        chunk->west->east = chunk;
        MarkChunkDirty(world, chunk->west);
    }

    chunk->north = HashMapFind(&world->chunks_by_position, ChunkKey{.x=chunk->x, .z=(s16)(chunk->z+1)});
    if (chunk->north)
    {
        chunk->north->south = chunk;
        MarkChunkDirty(world, chunk->north);
    }

    chunk->south = HashMapFind(&world->chunks_by_position, ChunkKey{.x=chunk->x, .z=(s16)(chunk->z-1)});
    if (chunk->south)
    {
        chunk->south->north = chunk;
        MarkChunkDirty(world, chunk->south);
    }

    auto work = Alloc<ChunkGenerationWork>(heap);
    work->world = world;
    work->chunk = chunk;

    AddWork(&world->chunk_generation_thread_group, work);
}

void GenerateChunkWorker(ThreadGroup *worker, void *data)
{
    auto work = (ChunkGenerationWork *)data;

    World *world = work->world;
    Chunk *chunk = work->chunk;

    // Fill surface level terrain params
    for (int iz = 0; iz < Chunk_Size; iz += 1)
    {
        for (int ix = 0; ix < Chunk_Size; ix += 1)
        {
            int surface_index = iz * Chunk_Size + ix;

            float perlin_x = (chunk->x * Chunk_Size + ix);
            float perlin_z = (chunk->z * Chunk_Size + iz);

            float continentalness = PerlinFractalNoise(world->continentalness_params, world->continentalness_offsets, perlin_x, perlin_z);
            float erosion = PerlinFractalNoise(world->erosion_params, world->erosion_offsets, perlin_x, perlin_z);
            erosion = (erosion + 1) * 0.5;
            float peaks_and_valleys = PerlinFractalNoise(world->peaks_and_valleys_params, world->peaks_and_valleys_offsets, perlin_x, perlin_z);
            peaks_and_valleys = 1 - Abs(3 * Abs(peaks_and_valleys) - 2);

            chunk->continentalness_values[surface_index] = continentalness;
            chunk->erosion_values[surface_index] = erosion;
            chunk->peaks_and_valleys_values[surface_index] = peaks_and_valleys;

            float erosion_spline = SampleSpline(&world->erosion_spline, erosion);
            float continentalness_spline = SampleSpline(&world->continentalness_spline, continentalness);

            float terrain_height = Lerp(continentalness_spline, Water_Level, erosion_spline);
            chunk->terrain_height_values[surface_index] = (u8)Clamp(terrain_height, 0, Chunk_Height - 1);
        }
    }

    // Terrain shaping
    for (int iy = 0; iy < Chunk_Height; iy += 1)
    {
        for (int iz = 0; iz < Chunk_Size; iz += 1)
        {
            for (int ix = 0; ix < Chunk_Size; ix += 1)
            {
                int surface_index = iz * Chunk_Size + ix;
                int index = iy * Chunk_Size * Chunk_Size + surface_index;

                float perlin_x = (chunk->x * Chunk_Size + ix);
                float perlin_y = iy;
                float perlin_z = (chunk->z * Chunk_Size + iz);

                float continentalness = chunk->continentalness_values[surface_index];
                float erosion = chunk->erosion_values[surface_index];
                float peaks_and_valleys = chunk->peaks_and_valleys_values[surface_index];
                float base_height = chunk->terrain_height_values[surface_index];

                float density = PerlinFractalNoise(world->density_params, world->density_offsets, perlin_x, perlin_y, perlin_z);
                float density_bias = (base_height - iy) * squashing_factor * squashing_factor;
                chunk->density_values[index] = density;

                if (iy <= base_height)
                    chunk->blocks[index] = Block_Stone;
                else if (iy <= Water_Level)
                    chunk->blocks[index] = Block_Water;
                else
                    chunk->blocks[index] = Block_Air;
            }
        }
    }

    // Terrain features (grass, dirt, etc)
    for (int iy = 0; iy < Chunk_Height; iy += 1)
    {
        for (int iz = 0; iz < Chunk_Size; iz += 1)
        {
            for (int ix = 0; ix < Chunk_Size; ix += 1)
            {
                int surface_index = iz * Chunk_Size + ix;
                int index = iy * Chunk_Size * Chunk_Size + surface_index;
                if (chunk->blocks[index] != Block_Stone)
                    continue;

                float terrain_height = chunk->terrain_height_values[surface_index];

                if (iy < Water_Level)
                {
                    if (iy >= terrain_height - Underwater_Gravel_Layer_Size)
                        chunk->blocks[index] = Block_Gravel;
                }
                else
                {
                    if (iy == terrain_height)
                        chunk->blocks[index] = Block_Grass;
                    else if (iy >= terrain_height - Dirt_Layer_Size)
                        chunk->blocks[index] = Block_Dirt;
                }
            }
        }
    }
}

void HandleNewlyGeneratedChunks(World *world)
{
    auto completed = GetCompletedWork(&world->chunk_generation_thread_group);
    foreach (i, completed)
    {
        auto work = (ChunkGenerationWork *)completed[i];

        work->chunk->is_generated = true;
        world->num_generated_chunks += 1;
        MarkChunkDirty(world, work->chunk);
        Free(work, heap);
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
