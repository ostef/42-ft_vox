#include "Core.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "Input.hpp"
#include "UI.hpp"

#include <SDL.h>

u64 g_frame_index = 0;

static MemoryArena g_frame_arena;

Allocator heap = Allocator{null, HeapAllocator};
Allocator temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

SDL_Window *g_window;
World g_world;

static GfxTexture GenerateNoiseTexture(String name, u32 seed, NoiseParams params, u32 size)
{
    GfxTextureDesc desc{};
    desc.type = GfxTextureType_Texture2D;
    desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
    desc.width = size;
    desc.height = size;
    desc.usage = GfxTextureUsage_ShaderRead;
    auto texture = GfxCreateTexture(name, desc);

    RNG rng{};
    RandomSeed(&rng, seed);

    Slice<Vec2f> offsets = AllocSlice<Vec2f>(params.octaves, heap);
    PerlinGenerateOffsets(&rng, &offsets);

    u32 *pixels = Alloc<u32>(size * size, heap);
    for (u32 y = 0; y < size; y += 1)
    {
        for (u32 x = 0; x < size; x += 1)
        {
            float noise = PerlinFractalNoise(params, offsets, (float)x, (float)y);
            noise = (noise + 1) * 0.5;
            u32 a = (u32)Clamp(noise * 255, 0, 255);
            pixels[y * size + x] = (0xff << 24) | (a << 16) | (a << 8) | a;
        }
    }

    GfxReplaceTextureRegion(&texture, {0,0,0}, {size, size, 1}, 0, 0, pixels);

    return texture;
}

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
    for (s16 x = -2; x < 3; x += 1)
    {
        for (s16 z = -2; z < 3; z += 1)
        {
            GenerateChunk(&g_world, x, z);
        }
    }

    NoiseParams noise_params{};
    noise_params.scale = 0.05;
    noise_params.octaves = 5;
    noise_params.persistance = 0.5;
    noise_params.max_amplitude = PerlinFractalMax(noise_params.octaves, noise_params.persistance);
    auto noise_texture = GenerateNoiseTexture("Noise", 123456, noise_params, 256);

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

        UIBeginFrame();
        UIImage(10, 10, 256, 256, &noise_texture);

        UpdateCamera(&g_world.camera);

        RenderGraphics(&g_world);
    }
}
