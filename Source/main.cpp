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

static const char *Noise_Param_Names[] = {
    "Density",
    "Continentalness",
    "Erosion",
    "Peaks And Valleys",
};

static GfxTexture GenerateNoiseTexture(World *world, int param, u32 size)
{
    String name = Noise_Param_Names[param];
    auto all_params = GetAllNoiseParams(world);

    GfxTextureDesc desc{};
    desc.type = GfxTextureType_Texture2D;
    desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
    desc.width = size;
    desc.height = size;
    desc.usage = GfxTextureUsage_ShaderRead;
    auto texture = GfxCreateTexture(name, desc);

    u32 *pixels = Alloc<u32>(size * size, heap);
    for (u32 y = 0; y < size; y += 1)
    {
        for (u32 x = 0; x < size; x += 1)
        {
            float ix = x - size * 0.5;
            float iy = y - size * 0.5;

            float noise;
            if (param == 0)
                noise = PerlinFractalNoise(all_params[param], world->density_offsets, ix, 0, iy);
            else
                noise = PerlinFractalNoise(all_params[param], (&world->continentalness_offsets)[param - 1], ix, iy);

            if (param == 3)
                noise = Abs(noise);
            else
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
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    #elif defined(VOX_BACKEND_VULKAN)
        sdl_flags |= SDL_WINDOW_VULKAN;
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
    #elif defined(VOX_BACKEND_METAL)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    #endif

    g_window = SDL_CreateWindow("Vox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, sdl_flags);
    defer(SDL_DestroyWindow(g_window));

    GfxCreateContext(g_window);
    defer(GfxDestroyContext());

    LoadAllShaders();
    InitRenderer();

    SetDefaultNoiseParams(&g_world);
    InitWorld(&g_world, 123456);
    defer(DestroyWorld(&g_world));

    int N = 3;
    for (s16 x = -N; x <= N; x += 1)
    {
        for (s16 z = -N; z <= N; z += 1)
        {
            GenerateChunk(&g_world, x, z);
        }
    }

    GfxTexture noise_texture = {};

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

        bool regenerate = false;

        UIBeginFrame();

        auto all_noise_params = GetAllNoiseParams(&g_world);

        bool regenerate_noise = false;
        if (UIButton("Reset to default"))
        {
            SetDefaultNoiseParams(&g_world);
            regenerate_noise = true;
            regenerate = true;
        }

        if (UIButton("<#prev_noise"))
        {
            current_params -= 1;
            if (current_params < 0)
                current_params = all_noise_params.count - 1;
            regenerate_noise = true;
        }

        UISameLine();

        if (UIButton(">#next_noise"))
        {
            current_params += 1;
            if (current_params >= all_noise_params.count)
                current_params = 0;
            regenerate_noise = true;
        }

        UISameLine();

        UIText(Noise_Param_Names[current_params]);

        if (UINoiseParams(Noise_Param_Names[current_params], &all_noise_params[current_params]))
        {
            regenerate = true;
            regenerate_noise = true;
        }

        UIImage(&noise_texture, {200, 200});

        if (UIButton("-#squashing_factor"))
        {
            squashing_factor -= IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.01 : 0.1;
            regenerate = true;
        }
        UISameLine();
        if (UIButton("+#squashing_factor"))
        {
            squashing_factor += IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.01 : 0.1;
            regenerate = true;
        }
        UISameLine();
        squashing_factor = Clamp(squashing_factor, 0, 1);
        UIText(TPrintf("squashing_factor: %.2f", squashing_factor));

        if (regenerate)
        {
            Camera camera = g_world.camera;

            DestroyWorld(&g_world);
            InitWorld(&g_world, g_world.seed);

            g_world.camera = camera;

            for (s16 x = -N; x <= N; x += 1)
            {
                for (s16 z = -N; z <= N; z += 1)
                {
                    GenerateChunk(&g_world, x, z);
                }
            }
        }

        if (regenerate_noise || IsNull(&noise_texture))
        {
            if (!IsNull(&noise_texture))
                GfxDestroyTexture(&noise_texture);

            noise_texture = GenerateNoiseTexture(&g_world, current_params, Chunk_Size * N * 2);
        }

        UpdateCamera(&g_world.camera);

        RenderGraphics(&g_world);
    }
}
