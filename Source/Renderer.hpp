#pragma once

#include "Core.hpp"
#include "Graphics.hpp"

struct ShaderFile
{
    String name = "";
    String vertex_entry_point = "VertexMain";
    String fragment_entry_point = "FragmentMain";
    GfxPipelineStageFlags stages = 0;
    GfxShader vertex_shader = {};
    GfxShader fragment_shader = {};
};

void LoadAllShaders();

GfxShader *GetVertexShader(String name);
GfxShader *GetFragmentShader(String name);
