#include "Core.hpp"
#include "Graphics.hpp"
#include "Graphics/Renderer.hpp"
#include "World.hpp"
#include "Input.hpp"
#include "UI.hpp"

#include <SDL.h>

Settings g_settings;

u64 g_frame_index = 0;

static MemoryArena g_frame_arena;

Allocator heap = Allocator{null, HeapAllocator};
Allocator temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

SDL_Window *g_window;
World g_world;

int main(int argc, char **args)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    u32 sdl_flags = SDL_WINDOW_RESIZABLE;
    #if defined(VOX_BACKEND_OPENGL)
        sdl_flags |= SDL_WINDOW_OPENGL;
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    #elif defined(VOX_BACKEND_VULKAN)
        sdl_flags |= SDL_WINDOW_VULKAN;
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
    #elif defined(VOX_BACKEND_METAL)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    #endif

    g_window = SDL_CreateWindow("Vox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1800, 1012, sdl_flags);
    defer(SDL_DestroyWindow(g_window));

    GfxCreateContext(g_window);
    defer(GfxDestroyContext());

    LoadAllShaders();
    InitRenderer();

    SetDefaultNoiseParams(&g_world);
    InitWorld(&g_world, (u32)(GetTimeInSeconds() * 173894775));
    defer(DestroyWorld(&g_world));

    GfxTexture noise_texture = {};
    GfxTexture terrain_texture = {};

    int current_params = 1;
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

        HandleNewlyGeneratedChunks(&g_world);

        int window_w, window_h;
        SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

        UpdateCamera(&g_world.camera);
        UpdateUI(&g_world);

        GenerateChunksAroundPoint(&g_world, g_world.camera.position, g_settings.render_distance * Chunk_Size);

        RenderGraphics(&g_world);
    }
}
