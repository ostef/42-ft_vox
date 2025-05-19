#include "Core.hpp"
#include "Graphics.hpp"

#include <SDL.h>

static MemoryArena g_frame_arena;

Allocator heap = Allocator{null, HeapAllocator};
Allocator temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

SDL_Window *g_window;

static void RenderGraphics();

int main(int argc, char **args)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    u32 sdl_flags = 0;
    #if defined(VOX_BACKEND_OPENGL)
        sdl_flags |= SDL_WINDOW_OPENGL;
    #elif defined(VOX_BACKEND_VULKAN)
        sdl_flags |= SDL_WINDOW_VULKAN;
    #endif

    g_window = SDL_CreateWindow("Vox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, sdl_flags);
    defer(SDL_DestroyWindow(g_window));

    GfxCreateContext(g_window);
    defer(GfxDestroyContext());

    bool quit = false;
    while(!quit)
    {
        ResetMemoryArena(&g_frame_arena);

        SDL_Event event = {};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                quit = true;
        }

        RenderGraphics();
    }
}

void RenderGraphics()
{
    GfxBeginFrame();

    GfxCommandBuffer cmd_buffer = GfxCreateCommandBuffer("Frame");

    GfxRenderPassDesc pass_desc{};
    pass_desc.color_attachments[0] = GfxGetSwapchainTexture();
    GfxClearColor(&pass_desc, 0, {1.0, 0.1, 0.1, 1.0});
    auto pass = GfxBeginRenderPass("Test", &cmd_buffer, pass_desc);
    {
    }
    GfxEndRenderPass(&pass);

    GfxExecuteCommandBuffer(&cmd_buffer);

    GfxSubmitFrame();
}
