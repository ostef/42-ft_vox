#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#define Gfx_Backend GfxBackend_Metal

struct GfxBuffer
{
    MTL::Buffer *handle = null;
    GfxBufferDesc desc = {};
};

struct GfxTexture
{
    MTL::Texture *handle = null;
    GfxTextureDesc desc = {};
};

struct GfxSamplerState
{
    MTL::SamplerState *handle = null;
    GfxSamplerStateDesc desc = {};
};

struct GfxContext
{
    SDL_Window *window = null;
};

extern GfxContext g_gfx_context;

struct GfxCommandBuffer
{
    MTL::CommandBuffer *handle = null;
};

struct GfxShader
{
    MTL::Library *library = null;
    MTL::Function *function = null;
    GfxPipelineStage stage = GfxPipelineStage_Invalid;
    Slice<GfxPipelineBinding> bindings = {};
};

struct GfxPipelineState
{
    MTL::RenderPipelineState *handle = null;
    GfxPipelineStateDesc desc = {};

    Slice<GfxPipelineBinding> vertex_stage_bindings = {};
    Slice<GfxPipelineBinding> fragment_stage_bindings = {};
};

struct GfxRenderPass
{
    MTL::RenderCommandEncoder *encoder = null;

    String name = "";
    GfxRenderPassDesc desc = {};
    GfxCommandBuffer *cmd_buffer = null;
};

struct GfxCopyPass
{
    MTL::BlitCommandEncoder *encoder = null;

    String name = "";
    GfxCommandBuffer *cmd_buffer = null;
};


