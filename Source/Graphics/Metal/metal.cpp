#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "Graphics.hpp"

GfxContext g_gfx_context;

void GfxCreateContext(SDL_Window *window)
{
    g_gfx_context.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    g_gfx_context.metal_layer = (CA::MetalLayer *)SDL_RenderGetMetalLayer(g_gfx_context.renderer);
    g_gfx_context.device = g_gfx_context.metal_layer->device();
    g_gfx_context.cmd_queue = g_gfx_context.device->newCommandQueue();

    LogMessage(Log_Metal, "Initialized Metal: %s", g_gfx_context.device->name()->utf8String());
}

void GfxDestroyContext()
{
    g_gfx_context.cmd_queue->release();
    SDL_DestroyRenderer(g_gfx_context.renderer);
}

void GfxBeginFrame() {}
void GfxSubmitFrame() {}

s64 GfxGetBufferAlignment() { return 16; }
int GfxGetBackbufferIndex() { return 0; }
float GfxGetLastFrameGPUTime() { return 0; }

GfxTexture *GfxGetSwapchainTexture() { return null; }
GfxPixelFormat GfxGetSwapchainPixelFormat() { return GfxPixelFormat_Invalid; }

GfxCommandBuffer GfxCreateCommandBuffer(String name) { return {}; }
void GfxExecuteCommandBuffer(GfxCommandBuffer *cmd_buffer) {}
void GfxBeginDebugGroup(GfxCommandBuffer *cmd_buffer, String name) {}
void GfxEndDebugGroup(GfxCommandBuffer *cmd_buffer) {}
