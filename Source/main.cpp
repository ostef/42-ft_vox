#include "Core.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "Input.hpp"

#include <SDL.h>

u64 g_frame_index = 0;

static MemoryArena g_frame_arena;

Allocator heap = Allocator{null, HeapAllocator};
Allocator temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

SDL_Window *g_window;
World g_world;

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

    LoadAllShaders();
    InitRenderer();

    InitWorld(&g_world, 123456);
    for (s16 x = 0; x < 1; x += 1)
    {
        for (s16 z = 0; z < 1; z += 1)
        {
            GenerateChunk(&g_world, x, z);
        }
    }

    bool quit = false;
    while(!quit)
    {
        g_frame_index += 1;

        ResetMemoryArena(&g_frame_arena);

        UpdateInput();

        SDL_Event event = {};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                quit = true;

            HandleInputEvent(event);
        }

        UpdateCamera(&g_world.camera);

        RenderGraphics(&g_world);
    }
}
