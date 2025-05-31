#pragma once

#include "Core.hpp"
#include "Graphics/Renderer.hpp"

#define Num_Block_Meshes 2

enum Block : u8
{
    Block_Air,
    Block_Stone,
    Block_Dirt,
    Block_Grass,
    Block_Water,
    Block_Gravel,

    Block_Count,
};

struct BlockInfo
{
    String name = "";
    ChunkMeshType mesh_type = ChunkMeshType_Solid;
};

static const BlockInfo Block_Infos[] = {
    {.name="air", .mesh_type=ChunkMeshType_Air},
    {.name="stone"},
    {.name="dirt"},
    {.name="grass"},
    {.name="water", .mesh_type=ChunkMeshType_Translucent},
    {.name="gravel"},
};
