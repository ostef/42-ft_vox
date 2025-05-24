#pragma once

#include "Core.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"

#define Chunk_Height 256
#define Chunk_Size 16

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

struct World
{
    u32 seed;

    Camera camera {};
    HashMap<ChunkKey, Chunk *> chunks_by_position = {};
    Array<Chunk *> all_chunks = {};
    Array<Chunk *> dirty_chunks = {};
};

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
};

Block GetBlock(Chunk *chunk, int x, int y, int z);
Block GetBlockInNeighbors(Chunk *chunk, int x, int y, int z);

void InitWorld(World *world, u32 seed);
void GenerateChunk(World *world, s16 x, s16 z);

void MarkChunkDirty(World *world, Chunk *chunk);

struct WorldParams
{
    float squashing_factor = 0;
    float level_height = 0;
};

WorldParams SampleWorldParams(World *world, float x, float y, float z);
Block GetBlock(World *world, float x, float y, float z);
Block GetBlock(World *world, WorldParams params);
