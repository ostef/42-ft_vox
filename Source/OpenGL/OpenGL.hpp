#pragma once

#include <glad/glad.h>

struct GfxContext
{
    SDL_Window *window = null;
    SDL_GLContext gl_context = null;
};

extern GfxContext g_gfx_context;

struct GfxBuffer
{
};

struct GfxTexture
{
};

struct GfxShader
{
};

struct GfxPipelineState
{
};

struct GfxRenderPass
{
};
