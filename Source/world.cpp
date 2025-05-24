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

void UpdateCamera(Camera *camera)
{
    bool moving = IsMouseButtonDown(MouseButton_Right);
    SDL_SetRelativeMouseMode((SDL_bool)moving);

    if (moving)
    {
        if (IsKeyDown(SDL_SCANCODE_A))
            camera->position.x -= camera->base_speed;
        if (IsKeyDown(SDL_SCANCODE_D))
            camera->position.x += camera->base_speed;
        if (IsKeyDown(SDL_SCANCODE_E))
            camera->position.y += camera->base_speed;
        if (IsKeyDown(SDL_SCANCODE_Q))
            camera->position.y -= camera->base_speed;
        if (IsKeyDown(SDL_SCANCODE_W))
            camera->position.z += camera->base_speed;
        if (IsKeyDown(SDL_SCANCODE_S))
            camera->position.z -= camera->base_speed;

        Vec2f rotation = GetMouseDelta() * camera->rotation_speed;

        camera->target_yaw += ToRads(rotation.x);
        camera->target_pitch += ToRads(rotation.y);
        camera->target_pitch = Clamp(camera->target_pitch, ToRads(-90), ToRads(90));

        camera->current_yaw = Lerp(camera->current_yaw, camera->target_yaw, camera->rotation_smoothing);
        camera->current_pitch = Lerp(camera->current_pitch, camera->target_pitch, camera->rotation_smoothing);
    }

    CalculateCameraMatrices(camera);
}

void CalculateCameraMatrices(Camera *camera)
{
    int width, height;
    SDL_GetWindowSizeInPixels(g_window, &width, &height);
    float aspect = width / (float)height;

    Mat4f rotation = Mat4fRotate({0,1,0}, camera->current_yaw) * Mat4fRotate({1,0,0}, camera->current_pitch);
    camera->transform = Mat4fTranslate(camera->position) * rotation;
    camera->view = Inverted(camera->transform);
    camera->projection = Mat4fPerspectiveProjection(ToRads(camera->fov_in_degrees), aspect, camera->z_near_dist, camera->z_far_dist);
}
