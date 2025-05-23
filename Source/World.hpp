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

struct World
{
    u32 seed;
    HashMap<ChunkKey, Chunk *> chunks_by_position = {};
    Array<Chunk *> dirty_chunks = {};
};

enum Block : u8
{
    Block_Air,
    Block_Stone,
    Block_Dirt,
    Block_Grass,
};

static const char *Block_Names[] = {
    "air",
    "stone",
    "dirt",
    "grass",
};

struct Chunk
{
    s16 x, z;

    bool is_generated = false;
    Mesh mesh = {};

    Block blocks[Chunk_Height * Chunk_Size * Chunk_Size];
};

void InitWorld(World *world, u32 seed);
void GenerateChunk(World *world, s16 x, s16 z);
