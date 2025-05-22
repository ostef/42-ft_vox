#pragma once

#include <glad/glad.h>

#define Gfx_Backend GfxBackend_OpenGL

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

struct OpenGLFramebufferKey
{
    GLuint color_textures[Gfx_Max_Color_Attachments] = {};
    GLuint depth_texture = 0;
    GLuint stencil_texture = 0;
};

struct GfxContext
{
    SDL_Window *window = null;
    SDL_GLContext gl_context = null;

    // Represents the swapchain's texture. Not an actual valid texture because we cannot
    // get the swapchain texture in OpenGL. It must only be used to set as a render target
    GfxTexture dummy_swapchain_texture = {};

    HashMap<OpenGLFramebufferKey, GLuint> framebuffer_cache = {};

    int backbuffer_index = 0;

    float last_frame_gpu_time = 0;
    GLuint frame_time_queries[Gfx_Max_Frames_In_Flight * 2] = {};
};

extern GfxContext g_gfx_context;

struct GfxCommandBuffer
{
};

struct GfxShader
{
    GLuint handle = 0;
    GfxPipelineStage stage = GfxPipelineStage_Invalid;
    Slice<GfxPipelineBinding> bindings = {};
};

struct GfxPipelineState
{
};

struct GfxRenderPass
{
    const char *name = null;
    GfxRenderPassDesc desc = {};
    GfxCommandBuffer *cmd_buffer = null;

    GLuint fbo = 0;
};
