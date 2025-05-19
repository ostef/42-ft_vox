#pragma once

#include "Core.hpp"
#include "Math.hpp"

#include <SDL.h>

#define Gfx_Max_Frames_In_Flight 2

struct GfxContext;
struct GfxTexture;
struct GfxBuffer;
struct GfxShader;
struct GfxPipelineState;
struct GfxRenderPass;

void GfxCreateContext(SDL_Window *window);
void GfxDestroyContext();
void GfxBeginFrame();
void GfxSubmitFrame();

enum GfxCpuAccessFlags
{
    GfxCpuAccess_None  = 0x0,
    GfxCpuAccess_Read  = 0x1,
    GfxCpuAccess_Write = 0x2,
};

enum GfxBufferUsage
{
    GfxBufferUsage_None          = 0x0,
    GfxBufferUsage_UniformBuffer = 0x1,
    GfxBufferUsage_StorageBuffer = 0x2,
    GfxBufferUsage_VertexBuffer  = 0x4,
    GfxBufferUsage_IndexBuffer   = 0x8,
};

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

enum GfxTextureUsage
{
    GfxTextureUsage_Invalid = 0x0,
    GfxTextureUsage_RenderTarget = 0x1,
    GfxTextureUsage_DepthStencil = 0x2,
    GfxTextureUsage_ShaderRead = 0x4,
};

enum GfxPixelFormat
{
    GfxPixelFormat_Invalid,
    GfxPixelFormat_RGBAUnorm8,
    GfxPixelFormat_RGBAFloat32,
    GfxPixelFormat_DepthFloat32,
};

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

bool IsNull(GfxShader *shader);

GfxShader GfxLoadSpirVShader(String source_code, GfxPipelineStage stage);
void GfxDestroyShader(GfxShader *shader);

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

bool IsNull(GfxPipelineState *state);

GfxPipelineState GfxCreatePipelineState(String name, GfxPipelineStateDesc desc);
void GfxDestroyPipelineState(GfxPipelineState *state);

struct GfxRenderPassDesc
{
    GfxTexture *color_attachments[Gfx_Max_Color_Attachments] = {};
    GfxTexture *depth_attachment = null;

    bool should_clear_color[Gfx_Max_Color_Attachments] = {};
    bool should_clear_depth = false;

    Vec4f clear_color[Gfx_Max_Color_Attachments] = {};
    float clear_depth = 0;
};

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

GfxRenderPass GfxBeginRenderPass(String name, GfxRenderPassDesc desc);
void GfxEndRenderPass(GfxRenderPass *pass);

void GfxSetPipelineState(GfxRenderPass *pass, GfxPipelineState *state);
void GfxSetViewport(GfxRenderPass *pass, GfxViewport viewport);
void GfxSetScissorRect(GfxRenderPass *pass, Recti rect);

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
