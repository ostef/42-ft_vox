#include "Graphics.hpp"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

void GfxCreateContext(SDL_Window *window) {}
void GfxDestroyContext() {}

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
