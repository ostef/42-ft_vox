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
    defer(DestroyWorld(&g_world));

    for (s16 x = -2; x < 3; x += 1)
    {
        for (s16 z = -2; z < 3; z += 1)
        {
            GenerateChunk(&g_world, x, z);
        }
    }

    auto noise_texture = GenerateNoiseTexture("Continentalness", 123456, g_world.continentalness_params, 256);

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

        bool regenerate = false;
        if (IsKeyPressed(SDL_SCANCODE_1))
        {
            base_height -= 5;
            base_height = Clamp(base_height, 0, Chunk_Height - 1);
            regenerate = true;
        }
        if (IsKeyPressed(SDL_SCANCODE_2))
        {
            base_height += 5;
            base_height = Clamp(base_height, 0, Chunk_Height - 1);
            regenerate = true;
        }
        if (IsKeyPressed(SDL_SCANCODE_3))
        {
            squashing_factor -= 0.1;
            squashing_factor = Clamp(squashing_factor, 0, 1);
            regenerate = true;
        }
        if (IsKeyPressed(SDL_SCANCODE_4))
        {
            squashing_factor += 0.1;
            squashing_factor = Clamp(squashing_factor, 0, 1);
            regenerate = true;
        }

        if (regenerate)
        {
            Camera camera = g_world.camera;

            DestroyWorld(&g_world);
            InitWorld(&g_world, 123456);

            g_world.camera = camera;

            for (s16 x = -2; x < 3; x += 1)
            {
                for (s16 z = -2; z < 3; z += 1)
                {
                    GenerateChunk(&g_world, x, z);
                }
            }
        }

        UIBeginFrame();
        UIImage(10, 10, 256, 256, &noise_texture);
        UIText(10, 300, TPrintf("squashing_factor: %.2f\nbase_height: %.2f", squashing_factor, base_height));

        UpdateCamera(&g_world.camera);

        RenderGraphics(&g_world);
    }
}
