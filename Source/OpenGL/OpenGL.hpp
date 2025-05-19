#pragma once

#include <glad/glad.h>

struct GfxTexture
{
    GLuint handle = 0;
    GfxTextureDesc desc = {};
};

struct GfxBuffer
{
    GLuint handle = 0;
    GfxBufferDesc desc = {};
};

struct GfxContext
{
    SDL_Window *window = null;
    SDL_GLContext gl_context = null;

    // Represents the swapchain's texture. Not an actual valid texture because we cannot
    // get the swapchain texture in OpenGL. It must only be used to set as a render target
    GfxTexture dummy_swapchain_texture = {};

    int backbuffer_index = 0;

    float last_frame_gpu_time = 0;
    GLuint frame_time_queries[Gfx_Max_Frames_In_Flight * 2] = {};
};

extern GfxContext g_gfx_context;

struct GfxShader
{
};

struct GfxPipelineState
{
};

struct GfxRenderPass
{
};
