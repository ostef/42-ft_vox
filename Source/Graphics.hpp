#pragma once

// Graphics abstraction layer

#include "Core.hpp"
#include "Math.hpp"

#include <SDL.h>

#define Gfx_Max_Frames_In_Flight 2
#define Gfx_Break_On_Error

struct GfxContext;
struct GfxCommandBuffer;
struct GfxTexture;
struct GfxBuffer;
struct GfxShader;
struct GfxPipelineState;
struct GfxRenderPass;

enum GfxBackend
{
    GfxBackend_Invalid,
    GfxBackend_OpenGL,
    GfxBackend_Vulkan,
};

enum GfxPixelFormat
{
    GfxPixelFormat_Invalid,
    GfxPixelFormat_RGBAUnorm8,
    GfxPixelFormat_RGBAFloat32,
    GfxPixelFormat_DepthFloat32,
};

void GfxCreateContext(SDL_Window *window);
void GfxDestroyContext();

void GfxBeginFrame();
void GfxSubmitFrame();

int GfxGetBackbufferIndex();
float GfxGetLastFrameGPUTime();

GfxTexture *GfxGetSwapchainTexture();
GfxPixelFormat GfxGetSwapchainPixelFormat();

GfxCommandBuffer GfxCreateCommandBuffer(String name);
void GfxExecuteCommandBuffer(GfxCommandBuffer *cmd_buffer);
void GfxBeginDebugGroup(GfxCommandBuffer *cmd_buffer, String name);
void GfxEndDebugGroup(GfxCommandBuffer *cmd_buffer);

typedef uint32_t GfxCpuAccessFlags;
#define GfxCpuAccess_None  0x0
#define GfxCpuAccess_Read  0x1
#define GfxCpuAccess_Write 0x2

typedef uint32_t GfxBufferUsage;
#define GfxBufferUsage_None          0x0
#define GfxBufferUsage_UniformBuffer 0x1
#define GfxBufferUsage_StorageBuffer 0x2
#define GfxBufferUsage_VertexBuffer  0x4
#define GfxBufferUsage_IndexBuffer   0x8

struct GfxBufferDesc
{
    GfxBufferUsage usage = GfxBufferUsage_None;
    GfxCpuAccessFlags cpu_access = GfxCpuAccess_None;
    s64 size = 0;
};

bool IsNull(GfxBuffer *buffer);
GfxBufferDesc GetDesc(GfxBuffer *buffer);

GfxBuffer GfxCreateBuffer(String name, GfxBufferDesc desc);
void GfxDestroyBuffer(GfxBuffer *buffer);

void *GfxMapBuffer(GfxBuffer *buffer, s64 offset, s64 size);
void GfxUnmapBuffer(GfxBuffer *buffer);

enum GfxTextureType
{
    GfxTextureType_Invalid,
    GfxTextureType_Texture2D,
};

typedef uint32_t GfxTextureUsage;
#define GfxTextureUsage_Invalid      0x0
#define GfxTextureUsage_RenderTarget 0x1
#define GfxTextureUsage_DepthStencil 0x2
#define GfxTextureUsage_ShaderRead   0x4

struct GfxTextureDesc
{
    GfxTextureType type = GfxTextureType_Invalid;
    GfxPixelFormat pixel_format = GfxPixelFormat_Invalid;
    GfxTextureUsage usage = GfxTextureUsage_Invalid;
    u32 width = 0;
    u32 height = 0;
    u32 num_mipmap_levels = 1;
};

bool IsNull(GfxTexture *texture);
GfxTextureDesc GetDesc(GfxTexture *texture);

GfxTexture GfxCreateTexture(String name, GfxTextureDesc desc);
void GfxDestroyTexture(GfxTexture *texture);

enum GfxPipelineStage
{
    GfxPipelineStage_Invalid,
    GfxPipelineStage_Vertex,
    GfxPipelineStage_Fragment,
};

typedef uint32_t GfxPipelineStageFlags;
#define GfxPipelineStageFlag_Vertex   (1 << (uint32_t)GfxPipelineStage_Vertex)
#define GfxPipelineStageFlag_Fragment (1 << (uint32_t)GfxPipelineStage_Fragment)

enum GfxPrimitiveType
{
    GfxPrimitiveType_Invalid,
    GfxPrimitiveType_Point,
    GfxPrimitiveType_Line,
    GfxPrimitiveType_Triangle,
};

enum GfxPolygonFace
{
    GfxPolygonFace_None,
    GfxPolygonFace_Front,
    GfxPolygonFace_Back,
};

enum GfxPolygonWindingOrder
{
    GfxPolygonWindingOrder_Invalid,
    GfxPolygonWindingOrder_CCW,
    GfxPolygonWindingOrder_CW,
};

enum GfxFillMode
{
    GfxFillMode_Invalid,
    GfxFillMode_Fill,
    GfxFillMode_Lines,
};

struct GfxRasterizerStateDesc
{
    GfxPrimitiveType primitive_type = GfxPrimitiveType_Triangle;
    GfxPolygonFace cull_face = GfxPolygonFace_Back;
    GfxPolygonWindingOrder winding_order = GfxPolygonWindingOrder_CCW;
    GfxFillMode fill_mode = GfxFillMode_Fill;
};

enum GfxCompareFunc
{
    GfxCompareFunc_Invalid,
    GfxCompareFunc_Never,
    GfxCompareFunc_Less,
    GfxCompareFunc_LessEqual,
    GfxCompareFunc_Greater,
    GfxCompareFunc_GreaterEqual,
    GfxCompareFunc_Equal,
    GfxCompareFunc_NotEqual,
    GfxCompareFunc_Always,
};

struct GfxDepthStateDesc
{
    bool enabled = false;
    bool write_enabled = false;
    GfxCompareFunc compare_func = GfxCompareFunc_LessEqual;
};

enum GfxVertexFormat
{
    GfxVertexFormat_Invalid,

    GfxVertexFormat_Float,
    GfxVertexFormat_Float2,
    GfxVertexFormat_Float3,
    GfxVertexFormat_Float4,
};

struct GfxVertexInputDesc
{
    GfxVertexFormat format = GfxVertexFormat_Invalid;
    s64 offset = 0;
    s64 stride = 0;
    bool normalized = false;
    u32 buffer_index = 0;
};

enum GfxBlendFactor
{
    GfxBlendFactor_Invalid,

    GfxBlendFactor_One,
    GfxBlendFactor_Zero,

    GfxBlendFactor_SrcColor,
    GfxBlendFactor_OneMinusSrcColor,
    GfxBlendFactor_SrcAlpha,
    GfxBlendFactor_OneMinusSrcAlpha,

    GfxBlendFactor_DstColor,
    GfxBlendFactor_OneMinusDstColor,
    GfxBlendFactor_DstAlpha,
    GfxBlendFactor_OneMinusDstAlpha,
};

enum GfxBlendOperation
{
    GfxBlendOperation_Invalid,
    GfxBlendOperation_Add,
    GfxBlendOperation_Subtract,
    GfxBlendOperation_ReverseSubtract,
    GfxBlendOperation_Min,
    GfxBlendOperation_Max,
};

struct GfxBlendStateDesc
{
    bool enabled = false;
    GfxBlendFactor src_RGB = GfxBlendFactor_SrcAlpha;
    GfxBlendFactor dst_RGB = GfxBlendFactor_OneMinusSrcAlpha;
    GfxBlendFactor src_alpha = GfxBlendFactor_SrcAlpha;
    GfxBlendFactor dst_alpha = GfxBlendFactor_OneMinusSrcAlpha;
    GfxBlendOperation RGB_operation = GfxBlendOperation_Add;
    GfxBlendOperation alpa_operation = GfxBlendOperation_Add;
};

#define Gfx_Max_Color_Attachments 8

struct GfxPipelineStateDesc
{
    GfxShader *vertex_shader = null;
    GfxShader *fragment_shader = null;
    Slice<GfxVertexInputDesc> vertex_layout = {};
    GfxRasterizerStateDesc rasterizer_state = {};
    GfxDepthStateDesc depth_state = {};
    GfxBlendStateDesc blend_states[Gfx_Max_Color_Attachments] = {};

    GfxPixelFormat color_formats[Gfx_Max_Color_Attachments] = {};
    GfxPixelFormat depth_format = GfxPixelFormat_Invalid;
};

enum GfxPipelineBindingType
{
    GfxPipelineBindingType_Invalid,
    GfxPipelineBindingType_UniformBuffer,
    GfxPipelineBindingType_StorageBuffer,
    GfxPipelineBindingType_Texture,
    GfxPipelineBindingType_SamplerState,
    GfxPipelineBindingType_CombinedSampler,
};

struct GfxPipelineBinding
{
    GfxPipelineBindingType type = GfxPipelineBindingType_Invalid;
    String name = {};
    int index = -1;
    Slice<int> associated_texture_units = {};
};

static inline Slice<GfxPipelineBinding> GfxCloneBindings(Slice<GfxPipelineBinding> bindings)
{
    Slice<GfxPipelineBinding> result = AllocSlice<GfxPipelineBinding>(bindings.count, heap);
    foreach (i, bindings)
    {
        auto b = bindings[i];
        auto res = &result[i];
        *res = b;
        res->name = CloneString(b.name, heap);
        res->associated_texture_units = AllocSlice<int>(b.associated_texture_units.count, heap);
        memcpy(res->associated_texture_units.data, b.associated_texture_units.data, sizeof(int) * b.associated_texture_units.count);
    }

    return result;
}

bool IsNull(GfxPipelineState *state);
GfxPipelineStateDesc GetDesc(GfxPipelineState *state);

GfxPipelineState GfxCreatePipelineState(String name, GfxPipelineStateDesc desc);
void GfxDestroyPipelineState(GfxPipelineState *state);

Slice<GfxPipelineBinding> GfxGetVertexStageBindings(GfxPipelineState *state);
Slice<GfxPipelineBinding> GfxGetFragmentStageBindings(GfxPipelineState *state);

bool IsNull(GfxShader *shader);

GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage, Slice<GfxPipelineBinding> bindings);
GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage);
void GfxDestroyShader(GfxShader *shader);

struct GfxRenderPassDesc
{
    GfxTexture *color_attachments[Gfx_Max_Color_Attachments] = {};
    GfxTexture *depth_attachment = null;
    GfxTexture *stencil_attachment = null;

    bool should_clear_color[Gfx_Max_Color_Attachments] = {};
    bool should_clear_depth = false;
    bool should_clear_stencil = false;

    Vec4f clear_color[Gfx_Max_Color_Attachments] = {};
    float clear_depth = 0;
    u32 clear_stencil = 0;
};

static inline void GfxSetColorAttachment(GfxRenderPassDesc *pass_desc, int index, GfxTexture *texture)
{
    Assert(index >= 0 && index < Gfx_Max_Color_Attachments);

    pass_desc->color_attachments[index] = texture;
}

static inline void GfxSetDepthAttachment(GfxRenderPassDesc *pass_desc, GfxTexture *texture)
{
    pass_desc->depth_attachment = texture;
}

static inline void GfxSetStencilAttachment(GfxRenderPassDesc *pass_desc, GfxTexture *texture)
{
    pass_desc->stencil_attachment = texture;
}

static inline void GfxClearColor(GfxRenderPassDesc *pass_desc, int index, Vec4f color)
{
    Assert(index >= 0 && index < Gfx_Max_Color_Attachments);

    pass_desc->should_clear_color[index] = true;
    pass_desc->clear_color[index] = color;
}

static inline void GfxClearDepth(GfxRenderPassDesc *pass_desc, float depth)
{
    pass_desc->should_clear_depth = true;
    pass_desc->clear_depth = depth;
}

static inline void GfxClearStencil(GfxRenderPassDesc *pass_desc, u32 stencil)
{
    pass_desc->should_clear_stencil = true;
    pass_desc->clear_stencil = stencil;
}

struct GfxViewport
{
    float top_left_x = 0;
    float top_left_y = 0;
    float width = 0;
    float height = 0;
    float min_depth = 0;
    float max_depth = 1;
};

enum GfxIndexType
{
    GfxIndexType_Uint16,
    GfxIndexType_Uint32,
};

GfxRenderPassDesc GetDesc(GfxRenderPass *pass);

GfxRenderPass GfxBeginRenderPass(String name, GfxCommandBuffer *cmd_buffer, GfxRenderPassDesc desc);
void GfxEndRenderPass(GfxRenderPass *pass);

// void GfxSetPipelineState(GfxRenderPass *pass, GfxPipelineState *state);
// void GfxSetViewport(GfxRenderPass *pass, GfxViewport viewport);
// void GfxSetScissorRect(GfxRenderPass *pass, Recti rect);

// void GfxSetVertexBuffer
// void GfxSetBuffer
// void GfxSetTexture
// void GfxSetSamplerState
// void GfxDrawPrimitives
// void GfxDrawIndexedPrimitives

#if defined(VOX_BACKEND_VULKAN)
#include "Vulkan/Vulkan.hpp"
#elif defined(VOX_BACKEND_OPENGL)
#include "OpenGL/OpenGL.hpp"
#else
#error "No graphics backend"
#endif
