#include "Core.hpp"

#include <SDL.h>

static MemoryArena g_frame_arena;

static Allocator g_heap = Allocator{null, HeapAllocator};
static Allocator g_temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

Allocator *heap = &g_heap;
Allocator *temp = &g_temp;

SDL_Window *g_window;

int main(int argc, char **args)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    g_window = SDL_CreateWindow("Vox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_VULKAN);
    defer(SDL_DestroyWindow(g_window));

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
    }
}
