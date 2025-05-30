#include "UI.hpp"
#include "Graphics/Renderer.hpp"
#include "World.hpp"
#include "Input.hpp"

#include <stb_image.h>

#define UI_Font_Char_Width 6
#define UI_Font_Char_Height 10
#define UI_Elem_Padding 6
#define UI_Button_Padding 3

static const char UI_Font_Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()[]{}<>/*:#%!?.,'\"@&$";

static bool g_ui_has_mouse;
static float g_ui_cursor_start_x;
static Vec2f g_ui_cursor;
static Vec2f g_ui_prev_elem_size;
static bool g_ui_same_line;

Array<UIRectElement> g_ui_elements;
GfxTexture g_ui_font;
GfxTexture g_ui_white_texture;

void UIBeginFrame()
{
    if (!g_ui_elements.allocator.func)
        g_ui_elements.allocator = heap;

    if (IsNull(&g_ui_font))
    {
        int width, height;
        u8 *pixels = stbi_load("Data/Fonts/minogram_6x10.png", &width, &height, null, 4);
        Assert(pixels != null, "Could not load font");

        GfxTextureDesc desc{};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
        desc.width = (u32)width;
        desc.height = (u32)height;
        desc.usage = GfxTextureUsage_ShaderRead;
        g_ui_font = GfxCreateTexture("UI Font", desc);
        Assert(!IsNull(&g_ui_font));

        GfxReplaceTextureRegion(&g_ui_font, {0,0,0}, {(u32)width,(u32)height,1}, 0, 0, pixels);
    }

    if (IsNull(&g_ui_white_texture))
    {
        GfxTextureDesc desc{};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
        desc.width = 1;
        desc.height = 1;
        desc.usage = GfxTextureUsage_ShaderRead;
        g_ui_white_texture = GfxCreateTexture("UI White", desc);
        Assert(!IsNull(&g_ui_white_texture));

        const u32 pixels[1] = {0xffffffff};
        GfxReplaceTextureRegion(&g_ui_white_texture, {0,0,0}, {1,1,1}, 0, 0, pixels);
    }

    ArrayClear(&g_ui_elements);
    g_ui_cursor = {};
    g_ui_cursor_start_x = 0;
    g_ui_prev_elem_size = {};
    g_ui_same_line = false;
}

void UISetMouse(bool has_mouse)
{
    g_ui_has_mouse = has_mouse;
}

void UISetCursorStart(float x, float y)
{
    g_ui_cursor_start_x = x;
    g_ui_cursor = {x, y};
}

static bool IsHovered(UIRectElement elem)
{
    Vec2f mouse_pos = GetMousePosition();

    return g_ui_has_mouse
        && mouse_pos.x > elem.position.x && mouse_pos.x < elem.position.x + elem.size.x
        && mouse_pos.y > elem.position.y && mouse_pos.y < elem.position.y + elem.size.y;
}

static Vec2f LayoutElem(Vec2f size)
{
    if (!g_ui_same_line)
    {
        g_ui_cursor.x = g_ui_cursor_start_x;
        g_ui_cursor.y += g_ui_prev_elem_size.y + UI_Elem_Padding;
    }

    g_ui_cursor.x += UI_Elem_Padding;

    Vec2f result = g_ui_cursor;

    g_ui_cursor.x += size.x;

    g_ui_prev_elem_size = size;
    g_ui_same_line = false;

    return result;
}

static Vec2f CalculateTextSize(String text)
{
    float w = 0;
    float x = 0;
    float y = 0;
    for (int i = 0; i < text.length; i += 1)
    {
        x += UI_Font_Char_Width * 2.0;
        if (text[i] == '\n')
        {
            x = 0;
            y += UI_Font_Char_Height * 2.0;
        }

        w = Max(w, x);
    }

    return {w, y + UI_Font_Char_Height * 2.0f};
}

static String GetIdText(String id)
{
    String text = id;
    for (int i = 0; i < text.length; i += 1)
    {
        if (text[i] == '#')
        {
            text.length = i;
            break;
        }
    }

    return text;
}

void UISameLine()
{
    g_ui_same_line = true;
}

void UIImage(GfxTexture *texture, Vec2f size, Vec2f uv0, Vec2f uv1)
{
    Vec2f position = LayoutElem(size);
    UIRectElement elem = {
        .position = position,
        .size = size,
        .texture = texture,
        .color = {1,1,1,1},
        .uv0 = uv0,
        .uv1 = uv1,
    };

    ArrayPush(&g_ui_elements, elem);
}

void UIText(String text)
{
    text = GetIdText(text);
    Vec2f text_size = CalculateTextSize(text);
    Vec2f position = LayoutElem(text_size);
    UITextAt(position, text);
}

void UITextAt(Vec2f position, String text)
{
    float scale = 2;
    float x_origin = position.x;
    float y_origin = position.y;
    float x = position.x;
    float y = position.y;

    for (int i = 0; i < text.length; i += 1)
    {
        int char_index = -1;
        for (int j = 0; j < (int)StaticArraySize(UI_Font_Chars); j += 1)
        {
            if (UI_Font_Chars[j] == text[i])
            {
                char_index = j;
                break;
            }
        }

        if (char_index >= 0)
        {
            UIRectElement elem = {};
            elem.position = {x,y};
            elem.size = Vec2f{UI_Font_Char_Width, UI_Font_Char_Height} * scale;
            elem.texture = &g_ui_font;
            elem.color = {1,1,1,1};

            int num_chars_x = GetDesc(&g_ui_font).width / UI_Font_Char_Width;
            int num_chars_y = GetDesc(&g_ui_font).height / UI_Font_Char_Height;
            elem.uv0.x = (char_index % num_chars_x) / (float)num_chars_x;
            elem.uv0.y = (char_index / num_chars_x) / (float)num_chars_y;
            elem.uv1.x = elem.uv0.x + 1 / (float)num_chars_x;
            elem.uv1.y = elem.uv0.y + 1 / (float)num_chars_y;

            ArrayPush(&g_ui_elements, elem);
        }

        if (text[i] == '\n')
        {
            x = x_origin;
            y += UI_Font_Char_Height * scale;
        }
        else
        {
            x += UI_Font_Char_Width * scale;
        }
    }
}

bool UIButton(String id)
{
    String text = GetIdText(id);

    float padding = UI_Button_Padding;
    Vec2f size = CalculateTextSize(text) + Vec2f{2 * padding, 2 * padding};

    UIRectElement bg{};
    bg.size = size;
    bg.position = LayoutElem(size);
    bg.color = {0, 0, 0, 1};

    bool hovered = IsHovered(bg);

    if (hovered)
        bg.color = {0.4, 0.4, 0.4, 1};

    ArrayPush(&g_ui_elements, bg);

    UITextAt(bg.position + Vec2f{padding, padding}, text);

    return hovered && IsMouseButtonReleased(MouseButton_Left);
}

bool UIFloatEdit(String id, float *value, float min, float max, float step)
{
    float old_value = *value;

    if (UIButton(TPrintf("-#%.*s", FSTR(id))))
        *value -= step * (IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.1 : IsKeyDown(SDL_SCANCODE_LALT) ? 10.0 : 1.0);
    UISameLine();
    if (UIButton(TPrintf("+#%.*s", FSTR(id))))
        *value += step * (IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.1 : IsKeyDown(SDL_SCANCODE_LALT) ? 10.0 : 1.0);
    UISameLine();

    *value = Clamp(*value, min, max);

    String text = GetIdText(id);
    UIText(TPrintf("%.*s: %.3f", FSTR(text), *value));

    return old_value != *value;
}

bool UIIntEdit(String id, int *value, int min, int max, int step)
{
    int old_value = *value;

    if (UIButton(TPrintf("-#%.*s", FSTR(id))))
        *value -= step * (IsKeyDown(SDL_SCANCODE_LSHIFT) ? 10 : 1);
    UISameLine();
    if (UIButton(TPrintf("+#%.*s", FSTR(id))))
        *value += step * (IsKeyDown(SDL_SCANCODE_LSHIFT) ? 10 : 1);
    UISameLine();

    *value = Clamp(*value, min, max);

    String text = GetIdText(id);
    UIText(TPrintf("%.*s: %d", FSTR(text), *value));

    return old_value != *value;
}

bool UINoiseParams(String id, NoiseParams *params)
{
    NoiseParams old = *params;

    float scale1000 = params->scale * 1000;
    if (UIFloatEdit(TPrintf("scale (x1000)#%.*s", FSTR(id)), &scale1000, 0, 1000, 0.1))
        params->scale = scale1000 / 1000;

    UIIntEdit(TPrintf("octaves#%.*s", FSTR(id)), &params->octaves, 1, Perlin_Fractal_Max_Octaves);
    UIFloatEdit(TPrintf("persistance#%.*s", FSTR(id)), &params->persistance, 0, 1, 0.1);
    UIFloatEdit(TPrintf("lacunarity#%.*s", FSTR(id)), &params->lacunarity, 0, 10, 0.1);

    params->max_amplitude = PerlinFractalMax(params->octaves, params->persistance);

    return memcmp(&old, params, sizeof(NoiseParams)) != 0;
}

struct SplineEditor
{
    Spline *spline = null;
    float min_x = 0;
    float max_x = 1;
    float min_y = 0;
    float max_y = 1;
    int selected_point = 0;
};

static void RecalculateMinMax(SplineEditor *editor, float min_x, float max_x, float min_y, float max_y)
{
    static const int Resolution = 10;

    if (!editor->spline)
        return;

    editor->min_x = INFINITY;
    editor->max_x = -INFINITY;
    editor->min_y = INFINITY;
    editor->max_y = -INFINITY;

    if (min_x != INFINITY && max_x != INFINITY)
    {
        editor->min_x = min_x;
        editor->max_x = max_x;
    }
    else
    {
        for (int i = 0; i < editor->spline->num_points; i += 1)
        {
            editor->min_x = Min(editor->min_x, editor->spline->points[i].x);
            editor->max_x = Max(editor->max_x, editor->spline->points[i].x);
        }
    }

    if (min_y != INFINITY && max_y != INFINITY)
    {
        editor->min_y = min_y;
        editor->max_y = max_y;
    }
    else
    {
        int iterations = editor->spline->num_points * Resolution;
        for (int i = 0; i < iterations; i += 1)
        {
            float t = Lerp(editor->min_x, editor->max_x, i / (float)(iterations - 1));
            float y = SampleSpline(editor->spline, t);
            editor->min_y = Min(editor->min_y, y);
            editor->max_y = Max(editor->max_y, y);
        }
    }
}

bool UISplineEditor(String id, Spline *spline, Vec2f size, float min_x, float max_x, float min_y, float max_y, float step_x, float step_y)
{
    static SplineEditor editor;
    static const int Resolution = 20;

    if (editor.spline != spline)
    {
        editor.spline = spline;
        editor.selected_point = 0;
        RecalculateMinMax(&editor, min_x, max_x, min_y, max_y);
    }

    UIText(id);

    UIRectElement bg = {};
    bg.size = size;
    bg.position = LayoutElem(size);
    bg.color = {0, 0, 0, 0.3};
    ArrayPush(&g_ui_elements, bg);

    bool point_is_hovered = false;
    float hovered_x, hovered_y;
    int iterations = spline->num_points * Resolution;
    for (int i = 0; i < iterations; i += 1)
    {
        float t = i / (float)(iterations - 1);
        float x = Lerp(editor.min_x, editor.max_x, t);
        float y = SampleSpline(spline, x);

        UIRectElement dot = {};
        dot.size = {2, 2};
        dot.position = bg.position + Vec2f{t * size.x, size.y - InverseLerp(editor.min_y, editor.max_y, y) * size.y};
        dot.position -= dot.size * 0.5;
        dot.color = {1,1,1,1};
        ArrayPush(&g_ui_elements, dot);

        dot.size += {4, 4};
        dot.position -= Vec2f{2,2};
        if (!point_is_hovered && IsHovered(dot))
        {
            point_is_hovered = true;
            hovered_x = x;
            hovered_y = y;
        }
    }

    for (int i = 0; i < spline->num_points; i += 1)
    {
        float x = InverseLerp(editor.min_x, editor.max_x, spline->points[i].x);
        float y = InverseLerp(editor.min_y, editor.max_y, spline->points[i].y);

        UIRectElement dot = {};
        dot.size = i == editor.selected_point ? Vec2f{8, 8} : Vec2f{5, 5};
        dot.position = bg.position + Vec2f{x * size.x, size.y - y * size.y};
        dot.position -= dot.size * 0.5;
        dot.color = i == editor.selected_point ? Vec4f{0.404, 1, 0.196, 1} : Vec4f{1, 0.863, 0.141, 1};
        ArrayPush(&g_ui_elements, dot);
    }

    if (point_is_hovered)
    {
        float x = InverseLerp(editor.min_x, editor.max_x, hovered_x);
        float y = InverseLerp(editor.min_y, editor.max_y, hovered_y);
        auto text = TPrintf("%.2f %.2f", hovered_x, hovered_y);

        UIRectElement text_bg = {};
        text_bg.size = CalculateTextSize(text) + Vec2f{4, 4};
        text_bg.position = bg.position + Vec2f{x * size.x, size.y - y * size.y} + Vec2f{10, -10};
        text_bg.color = {0,0,0,0.6};
        ArrayPush(&g_ui_elements, text_bg);

        UITextAt(text_bg.position + Vec2f{2, 2}, text);
    }

    bool modified = false;

    if (UIButton(TPrintf("<#%.*s", FSTR(id))))
    {
        editor.selected_point -= 1;
        if (editor.selected_point < 0)
            editor.selected_point = Max(spline->num_points - 1, 0);
    }

    UISameLine();
    UIText(TPrintf("%d", editor.selected_point));
    UISameLine();

    if (UIButton(TPrintf(">#%.*s", FSTR(id))))
    {
        editor.selected_point += 1;
        if (editor.selected_point >= spline->num_points)
            editor.selected_point = 0;
    }

    UISameLine();

    if (UIButton("Add"))
    {
        if (spline->num_points > 0)
        {
            auto p = spline->points[editor.selected_point];
            int new_index = AddPoint(spline, p.x, p.y, p.derivative);
            if (new_index >= 0)
                editor.selected_point = new_index;
        }
        else
        {
            editor.selected_point = AddPoint(spline, editor.min_x, editor.min_y, 0);
        }

        modified = true;
    }

    UISameLine();

    if (UIButton("Remove"))
    {
        if (spline->num_points > 0)
        {
            RemovePoint(spline, editor.selected_point);
            editor.selected_point = Min(editor.selected_point, spline->num_points - 1);
            editor.selected_point = Max(editor.selected_point, 0);
            modified = true;
        }
    }

    if (spline->num_points > 0)
    {
        auto selected_point = &spline->points[editor.selected_point];
        float selected_point_min_x = editor.selected_point == 0 ? editor.min_x - step_x * 10 : spline->points[editor.selected_point - 1].x;
        float selected_point_max_x = editor.selected_point == spline->num_points - 1 ? editor.max_x + step_x * 10 : spline->points[editor.selected_point + 1].x;
        if (UIFloatEdit("X", &selected_point->x, selected_point_min_x, selected_point_max_x, step_x))
        {
            RecalculateMinMax(&editor, min_x, max_x, min_y, max_y);
            modified = true;
        }

        if (UIFloatEdit("Y", &selected_point->y, editor.min_y - 1, editor.max_y + 1, step_y))
        {
            RecalculateMinMax(&editor, min_x, max_x, min_y, max_y);
            modified = true;
        }

        if (UIFloatEdit("dX/dY", &selected_point->derivative, -1000, 1000))
        {
            RecalculateMinMax(&editor, min_x, max_x, min_y, max_y);
            modified = true;
        }

        UISameLine();

        if (UIButton(TPrintf("Reset#derivative_%.*s", FSTR(id))))
        {
            selected_point->derivative = 0;
            RecalculateMinMax(&editor, min_x, max_x, min_y, max_y);
            modified = true;
        }
    }

    return modified;
}

static const char *Noise_Param_Names[] = {
    "Density",
    "Continentalness",
    "Erosion",
    "Peaks And Valleys",
};

static GfxTexture GenerateTerrainTexture(World *world, int size_in_chunks)
{
    int pixel_size = size_in_chunks * 2 * Chunk_Size;
    u32 *pixels = Alloc<u32>(pixel_size * pixel_size, heap);

    for (int z = -size_in_chunks; z < size_in_chunks; z += 1)
    {
        for (int x = -size_in_chunks; x < size_in_chunks; x += 1)
        {
            int px_min_x = (x + size_in_chunks) * Chunk_Size;
            int px_max_x = px_min_x + Chunk_Size;
            int px_min_y = (z + size_in_chunks) * Chunk_Size;
            int px_max_y = px_min_y + Chunk_Size;

            Chunk *chunk = HashMapFind(&world->chunks_by_position, {(s16)x, (s16)z});
            if (!chunk)
            {
                for (int px_y = px_min_y; px_y < px_max_y; px_y += 1)
                {
                    for (int px_x = px_min_x; px_x < px_max_x; px_x += 1)
                    {
                        int px_index = px_y * pixel_size + px_x;
                        pixels[px_index] = 0xff000000;
                    }
                }
            }
            else
            {
                for (int px_y = px_min_y; px_y < px_max_y; px_y += 1)
                {
                    for (int px_x = px_min_x; px_x < px_max_x; px_x += 1)
                    {
                        int px_index = px_y * pixel_size + px_x;

                        int chunk_x = px_x - px_min_x;
                        int chunk_z = px_y - px_min_y;
                        int surface_index = chunk_z * Chunk_Size + chunk_x;
                        float terrain_height = chunk->terrain_height_values[surface_index];
                        terrain_height /= (float)Chunk_Height;
                        if (terrain_height > Water_Level / (float)Chunk_Height)
                        {
                            u32 a = (u32)Clamp(terrain_height * 255, 0, 255);
                            pixels[px_index] = (0xff << 24) | (a << 16) | (a << 8) | a;
                        }
                        else
                        {
                            Vec4f color = {0,0,1,1};
                            color *= terrain_height / (Water_Level / (float)Chunk_Height);

                            u32 r = (u32)Clamp(color.x * 255, 0, 255);
                            u32 g = (u32)Clamp(color.y * 255, 0, 255);
                            u32 b = (u32)Clamp(color.z * 255, 0, 255);

                            pixels[px_index] = (0xff << 24) | (b << 16) | (g << 8) | r;
                        }
                    }
                }
            }
        }
    }

    GfxTextureDesc desc{};
    desc.type = GfxTextureType_Texture2D;
    desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
    desc.width = (u32)pixel_size;
    desc.height = (u32)pixel_size;
    desc.usage = GfxTextureUsage_ShaderRead;
    auto texture = GfxCreateTexture("Terrain", desc);

    GfxReplaceTextureRegion(&texture, {0,0,0}, {desc.width, desc.height, 1}, 0, 0, pixels);

    return texture;
}

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
                noise = 1 - Abs(3 * Abs(noise) - 2);

            noise = (noise + 1) * 0.5;

            u32 a = (u32)Clamp(noise * 255, 0, 255);
            pixels[y * size + x] = (0xff << 24) | (a << 16) | (a << 8) | a;
        }
    }

    GfxReplaceTextureRegion(&texture, {0,0,0}, {size, size, 1}, 0, 0, pixels);

    return texture;
}

static void WriteSplineCSourceCode(String name, Spline *spline)
{
    printf("%.*s = {};\n", FSTR(name));
    for (int i = 0; i < spline->num_points; i += 1)
    {
        auto p = spline->points[i];
        printf("AddPoint(&%.*s, %.3f, %.3f, %.3f);\n", FSTR(name), p.x, p.y, p.derivative);
    }
    printf("\n");
}

static void ShowTerrainEditorUI(World *world)
{
    static int current_terrain_param;
    static GfxTexture noise_texture;

    bool regenerate = false;

    const char *Directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    float octant = world->camera.current_yaw / (2 * Pi);
    octant += 0.5 / 8.0;
    octant *= 8;
    octant = fmodf(octant, 8);
    if (octant < 0)
        octant += 8;
    if (octant >= 8)
        octant = 0;

    UIText(TPrintf("X: %.0f Y: %.0f Z: %.0f, %s", world->camera.position.x, world->camera.position.y, world->camera.position.z, Directions[(int)octant]));

    auto all_noise_params = GetAllNoiseParams(world);

    bool regenerate_noise = false;

    UIText(TPrintf("Seed: %u", world->seed));
    UISameLine();
    if (UIButton("Randomize"))
    {
        world->seed = (u32)(GetTimeInSeconds() * 347868765);
        regenerate = true;
        regenerate_noise = true;
    }

    if (UIButton("Reset to default"))
    {
        SetDefaultNoiseParams(world);
        regenerate_noise = true;
        regenerate = true;
    }

    if (UIButton("<#prev_noise"))
    {
        current_terrain_param -= 1;
        if (current_terrain_param < 0)
            current_terrain_param = all_noise_params.count - 1;
        regenerate_noise = true;
    }

    UISameLine();

    if (UIButton(">#next_noise"))
    {
        current_terrain_param += 1;
        if (current_terrain_param >= all_noise_params.count)
            current_terrain_param = 0;
        regenerate_noise = true;
    }

    UISameLine();

    UIText(Noise_Param_Names[current_terrain_param]);

    if (UINoiseParams(Noise_Param_Names[current_terrain_param], &all_noise_params[current_terrain_param]))
        regenerate_noise = true;

    UIFloatEdit("squashing_factor", &squashing_factor, 0, 1, 0.1);

    if (UIButton("Regenerate"))
        regenerate = true;

    UIImage(&noise_texture, {200, 200}, {0,1}, {1,0});

    if (current_terrain_param == 1)
    {
        UISplineEditor("Continentalness Spline", &world->continentalness_spline, {400, 250}, -1, 1, 0, Chunk_Height, 0.1, 1);

        if (UIButton("Dump to Terminal"))
            WriteSplineCSourceCode("world->continentalness_spline", &world->continentalness_spline);

        if (UIButton("Regenerate"))
            regenerate = true;
    }
    else if (current_terrain_param == 2)
    {
        UISplineEditor("Erosion Spline", &world->erosion_spline, {400, 250}, 0, 1, 0, 1, 0.1, 0.1);

        if (UIButton("Dump to Terminal"))
            WriteSplineCSourceCode("world->erosion_spline", &world->erosion_spline);

        if (UIButton("Regenerate"))
            regenerate = true;
    }

    // UIImage(&terrain_texture, {256, 256}, {0,1}, {1,0});

    if (regenerate)
    {
        Camera camera = world->camera;

        DestroyWorld(world);
        InitWorld(world, world->seed);

        world->camera = camera;
    }

    if (regenerate_noise || IsNull(&noise_texture))
    {
        if (!IsNull(&noise_texture))
            GfxDestroyTexture(&noise_texture);

        noise_texture = GenerateNoiseTexture(world, current_terrain_param, Chunk_Size * g_settings.render_distance * 2);
    }

    // if (regenerate && !IsNull(&terrain_texture))
    //     GfxDestroyTexture(&terrain_texture);

    // if (IsNull(&terrain_texture) && world->num_generated_chunks == world->all_chunks.count)
    //     terrain_texture = GenerateTerrainTexture(world, g_settings.render_distance);
}

static void ShowGraphicsEditorUI(World *world)
{
    UIText("== Shadow Map ==");

    int resolution = (int)GetDesc(&g_shadow_map_texture).width;
    if (UIIntEdit("resolution", &resolution, 128, 8192, 128))
        RecreateShadowMapTexture((u32)resolution);

    UIFloatEdit("depth extent factor", &g_shadow_map_depth_extent_factor, 1, 20);
    UIFloatEdit("forward offset", &g_shadow_map_forward_offset, 0, 1, 0.1);
    UIFloatEdit("min depth bias", &g_shadow_map_min_depth_bias, 0, 10);
    UIFloatEdit("max depth bias", &g_shadow_map_max_depth_bias, 0, 10);
    UIFloatEdit("normal bias", &g_shadow_map_normal_bias, 0, 1000, 5);
    UIFloatEdit("filter radius", &g_shadow_map_filter_radius, 0.1, 5, 0.1);

    for (int i = 0; i < Shadow_Map_Num_Cascades; i += 1)
        UIFloatEdit(TPrintf("cascade size [%d]", i), &g_shadow_map_cascade_sizes[i], 1, 500);
}

enum ActiveEditor
{
    ActiveEditor_Terrain,
    ActiveEditor_Graphics,
    ActiveEditor_Count,
};

static const char *Active_Editor_Names[] = {
    "Terrain", "Graphics",
};

void UpdateUI(World *world)
{
    static ActiveEditor active_editor;

    UIBeginFrame();

    if (UIButton("<#active_editor"))
    {
        active_editor = (ActiveEditor)((int)active_editor - 1);
        if ((int)active_editor < 0)
            active_editor = (ActiveEditor)((int)ActiveEditor_Count - 1);
    }

    UISameLine();
    UIText(Active_Editor_Names[active_editor]);
    UISameLine();

    if (UIButton(">#active_editor"))
    {
        active_editor = (ActiveEditor)((int)active_editor + 1);
        if ((int)active_editor == ActiveEditor_Count)
            active_editor = (ActiveEditor)0;
    }

    switch (active_editor)
    {
    case ActiveEditor_Terrain: ShowTerrainEditorUI(world); break;
    case ActiveEditor_Graphics: ShowGraphicsEditorUI(world); break;
    }
}
