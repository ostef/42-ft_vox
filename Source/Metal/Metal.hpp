#pragma once

// #include <Foundation/Foundation.hpp>
// #include <Metal/Metal.hpp>
// #include <QuartzCore/QuartzCore.hpp>

#define Gfx_Backend GfxBackend_Metal

struct GfxBuffer
{
    GfxBufferDesc desc = {};
};

struct GfxTexture
{
    GfxTextureDesc desc = {};
};

struct GfxSamplerState
{
    GfxSamplerStateDesc desc = {};
};

struct GfxContext
{
    SDL_Window *window = null;
};

extern GfxContext g_gfx_context;

struct GfxCommandBuffer
{
};

struct GfxShader
{
    GfxPipelineStage stage = GfxPipelineStage_Invalid;
    Slice<GfxPipelineBinding> bindings = {};
};

struct GfxPipelineState
{
    GfxPipelineStateDesc desc = {};

    Slice<GfxPipelineBinding> vertex_stage_bindings = {};
    Slice<GfxPipelineBinding> fragment_stage_bindings = {};
};

struct GfxRenderPass
{
    String name = "";
    GfxRenderPassDesc desc = {};
    GfxCommandBuffer *cmd_buffer = null;
};

struct GfxCopyPass
{
    String name = "";
    GfxCommandBuffer *cmd_buffer = null;
};


