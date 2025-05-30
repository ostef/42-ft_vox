#pragma once

#include "Core.hpp"

#define Num_Block_Meshes 2

enum Block : u8
{
    Block_Air,
    Block_Stone,
    Block_Dirt,
    Block_Grass,
    Block_Water,

    Block_Count,
};

struct BlockInfo
{
    String name = "";
    int mesh_id = 0;
};

static const BlockInfo Block_Infos[] = {
    {.name="air", .mesh_id=-1},
    {.name="stone"},
    {.name="dirt"},
    {.name="grass"},
    {.name="water", .mesh_id=1},
};
