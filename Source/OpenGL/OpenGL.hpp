#pragma once

#include <glad/glad.h>

#define Gfx_Backend GfxBackend_OpenGL

struct GfxBuffer
{
    GLuint handle = 0;
    GfxBufferDesc desc = {};
};

struct GfxTexture
{
    GLuint handle = 0;
    GfxTextureDesc desc = {};
};

struct GfxSamplerState
{
    GLuint handle = 0;
    GfxSamplerStateDesc desc = {};
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

struct OpenGLBindingRelocation
{
    int block_index_or_uniform_location = -1;
    int original_binding_index = -1;
    int relocated_binding_index = -1;
    Slice<OpenGLBindingRelocation> associated_texture_units = {};
};

struct GfxPipelineState
{
    GfxPipelineStateDesc desc = {};
    GLuint pso = 0;
    GLuint vao = 0;

    Slice<GfxPipelineBinding> vertex_stage_bindings = {};
    Slice<GfxPipelineBinding> fragment_stage_bindings = {};
    Slice<OpenGLBindingRelocation> fragment_stage_binding_relocations = {};
};

struct GfxRenderPass
{
    String name = "";
    GfxRenderPassDesc desc = {};
    GfxCommandBuffer *cmd_buffer = null;

    GLuint fbo = 0;
    GfxPipelineState *current_pipeline_state = null;
    GfxBuffer *current_index_buffer = null;
};

static const GLenum GL_Sampler_Types[] = {
    GL_SAMPLER_1D,
    GL_SAMPLER_2D,
    GL_SAMPLER_3D,
    GL_SAMPLER_CUBE,
    GL_SAMPLER_1D_SHADOW,
    GL_SAMPLER_2D_SHADOW,
    GL_SAMPLER_1D_ARRAY,
    GL_SAMPLER_2D_ARRAY,
    GL_SAMPLER_1D_ARRAY_SHADOW,
    GL_SAMPLER_2D_ARRAY_SHADOW,
    GL_SAMPLER_2D_MULTISAMPLE,
    GL_SAMPLER_2D_MULTISAMPLE_ARRAY,
    GL_SAMPLER_CUBE_SHADOW,
    GL_SAMPLER_BUFFER,
    GL_SAMPLER_2D_RECT,
    GL_SAMPLER_2D_RECT_SHADOW,
    GL_INT_SAMPLER_1D,
    GL_INT_SAMPLER_2D,
    GL_INT_SAMPLER_3D,
    GL_INT_SAMPLER_CUBE,
    GL_INT_SAMPLER_1D_ARRAY,
    GL_INT_SAMPLER_2D_ARRAY,
    GL_INT_SAMPLER_2D_MULTISAMPLE,
    GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
    GL_INT_SAMPLER_BUFFER,
    GL_INT_SAMPLER_2D_RECT,
    GL_UNSIGNED_INT_SAMPLER_1D,
    GL_UNSIGNED_INT_SAMPLER_2D,
    GL_UNSIGNED_INT_SAMPLER_3D,
    GL_UNSIGNED_INT_SAMPLER_CUBE,
    GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,
    GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
    GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,
    GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
    GL_UNSIGNED_INT_SAMPLER_BUFFER,
    GL_UNSIGNED_INT_SAMPLER_2D_RECT,
};

// Conversion functions
void GLFormatSizeAndType(GfxVertexFormat format, int *size, GLenum *type);

GLenum GLFillMode(GfxFillMode mode);
GLenum GLFace(GfxPolygonFace face);
GLenum GLWindingOrder(GfxPolygonWindingOrder order);
GLenum GLBlendFactor(GfxBlendFactor factor);
GLenum GLBlendEquation(GfxBlendOperation op);
GLenum GLComparisonFunc(GfxCompareFunc func);

GLenum GLTextureType(GfxTextureType type);
GLenum GLPixelFormat(GfxPixelFormat format);
void GLPixelFormatAndType(GfxPixelFormat pixel_format, GLenum *format, GLenum *type);
GLenum GLTextureFilter(GfxSamplerFilter filter, GfxSamplerFilter mip_filter);
GLenum GLTextureWrap(GfxSamplerAddressMode mode);
