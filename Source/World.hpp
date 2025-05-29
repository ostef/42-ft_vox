#pragma once

#include "Core.hpp"
#include "Graphics.hpp"
#include "Graphics/Renderer.hpp"

#define Chunk_Height 256
#define Chunk_Size 16

extern float squashing_factor;

struct Chunk;

struct ChunkKey
{
    s16 x, z;
};

struct Camera
{
    Vec3f position = {};
    Quatf rotation = {};

    float rotation_speed = 0.2;
    float rotation_smoothing = 0.3;
    float base_speed = 0.1;
    float fast_speed = 1.0;
    float speed_mult = 1.0;

    float target_yaw = 0;
    float current_yaw = 0;
    float target_pitch = 0;
    float current_pitch = 0;

    float fov_in_degrees = 80;
    float z_near_dist = 0.1;
    float z_far_dist = 1000.0;

    Mat4f transform = {};
    Mat4f view = {};
    Mat4f projection = {};
};

void UpdateCamera(Camera *camera);

#define Water_Level (Chunk_Height - 100)

struct World
{
    u32 seed;

    Vec3f sun_direction = {1,0,0};
    Camera camera {};
    HashMap<ChunkKey, Chunk *> chunks_by_position = {};
    Array<Chunk *> all_chunks = {};
    Array<Chunk *> dirty_chunks = {};
    int num_generated_chunks = 0;

    ThreadGroup chunk_generation_thread_group = {};
    ThreadGroup chunk_mesh_generation_thread_group = {};

    NoiseParams density_params = {};
    NoiseParams continentalness_params = {};
    NoiseParams erosion_params = {};
    NoiseParams peaks_and_valleys_params = {};
    Spline continentalness_spline = {};
    Spline erosion_spline = {};

    Slice<Vec3f> density_offsets = {};
    Slice<Vec2f> continentalness_offsets = {};
    Slice<Vec2f> erosion_offsets = {};
    Slice<Vec2f> peaks_and_valleys_offsets = {};
};

static inline Slice<NoiseParams> GetAllNoiseParams(World *world)
{
    return {.count=4, .data=&world->density_params};
}

enum Block : u8
{
    Block_Air,
    Block_Stone,
    Block_Dirt,
    Block_Grass,
    Block_Water,

    Block_Count,
};

static const char *Block_Names[] = {
    "air",
    "stone",
    "dirt",
    "grass",
    "water",
};

struct Chunk
{
    s16 x, z;

    bool is_generated = false;
    Mesh mesh = {};

    Chunk *east = null;
    Chunk *west = null;
    Chunk *north = null;
    Chunk *south = null;

    Block blocks[Chunk_Height * Chunk_Size * Chunk_Size];

    float continentalness_values[Chunk_Size * Chunk_Size];
    float erosion_values[Chunk_Size * Chunk_Size];
    float peaks_and_valleys_values[Chunk_Size * Chunk_Size];
    float terrain_height_values[Chunk_Size * Chunk_Size];
};

Block GetBlock(Chunk *chunk, int x, int y, int z);
Block GetBlockInNeighbors(Chunk *chunk, int x, int y, int z);

void SetDefaultNoiseParams(World *world);
void InitWorld(World *world, u32 seed);
void DestroyWorld(World *world);
void DestroyChunk(World *world, Chunk *chunk);

void GenerateChunksAroundPoint(World *world, Vec3f point, float radius);

void QueueChunkGeneration(World *world, s16 x, s16 z);
void HandleNewlyGeneratedChunks(World *world);

void MarkChunkDirty(World *world, Chunk *chunk);

struct ChunkMeshWork
{
    Array<BlockVertex> vertices = {};
    Array<u32> indices = {};
    Chunk *chunk = null;
};
