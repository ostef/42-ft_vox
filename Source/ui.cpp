#include "UI.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

#include <stb_image.h>

struct UIVertex
{
    Vec2f position;
    Vec2f tex_coords;
    Vec4f color;
};

struct UIRectElement
{
    Vec2f position = {};
    Vec2f size = {};
    GfxTexture *texture = null;
    Vec4f color = {};
    Vec2f uv0 = {};
    Vec2f uv1 = {1,1};
};

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

static Array<UIRectElement> g_ui_elements;
static GfxTexture g_ui_font;
static GfxTexture g_ui_white_texture;

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

static void PushRect(Array<UIVertex> *vertices, UIRectElement elem, int window_w, int window_h)
{
    Vec2f p = {elem.position.x, window_h - elem.position.y};
    auto v = ArrayPush(vertices);
    v->position = p;
    v->tex_coords = {elem.uv0.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = p + Vec2f{elem.size.x, -elem.size.y};
    v->tex_coords = {elem.uv1.x, elem.uv1.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = p + Vec2f{0, -elem.size.y};
    v->tex_coords = {elem.uv0.x, elem.uv1.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = p;
    v->tex_coords = {elem.uv0.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = p + Vec2f{elem.size.x, 0};
    v->tex_coords = {elem.uv1.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = p + Vec2f{elem.size.x, -elem.size.y};
    v->tex_coords = {elem.uv1.x, elem.uv1.y};
    v->color = elem.color;
}

static GfxPipelineState g_ui_pipeline;
static GfxSamplerState g_ui_texture_sampler;

static void InitPipeline()
{
    GfxPipelineStateDesc desc{};
    desc.rasterizer_state.cull_face = GfxPolygonFace_None;
    desc.color_formats[0] = GfxGetSwapchainPixelFormat();
    desc.blend_states[0] = {.enabled=true};
    desc.vertex_shader = GetVertexShader("ui");
    desc.fragment_shader = GetFragmentShader("ui");

    Array<GfxVertexInputDesc> vertex_layout = {.allocator=heap};
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_Float2,
        .offset=offsetof(UIVertex, position),
        .stride=sizeof(UIVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_Float2,
        .offset=offsetof(UIVertex, tex_coords),
        .stride=sizeof(UIVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    ArrayPush(&vertex_layout, {
        .format=GfxVertexFormat_Float4,
        .offset=offsetof(UIVertex, color),
        .stride=sizeof(UIVertex),
        .buffer_index=Default_Vertex_Buffer_Index
    });
    desc.vertex_layout = MakeSlice(vertex_layout);

    g_ui_pipeline = GfxCreatePipelineState("UI", desc);

    GfxSamplerStateDesc sampler_desc{};
    sampler_desc.min_filter = GfxSamplerFilter_Nearest;
    sampler_desc.mag_filter = GfxSamplerFilter_Nearest;
    g_ui_texture_sampler = GfxCreateSamplerState("UI", sampler_desc);
}

void UIRender(FrameRenderContext *ctx)
{
    if (IsNull(&g_ui_pipeline))
    {
        InitPipeline();
        Assert(!IsNull(&g_ui_pipeline));
    }

    if (g_ui_elements.count <= 0)
        return;

    int window_w, window_h;
    SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

    // Allocate vertices
    Array<UIVertex> vertices{};
    vertices.allocated = g_ui_elements.count * 6;
    vertices.data = Alloc<UIVertex>(vertices.allocated, FrameDataAllocator());
    foreach (i, g_ui_elements)
        PushRect(&vertices, g_ui_elements[i], window_w, window_h);

    s64 vertices_offset = GetBufferOffset(FrameDataGfxAllocator(), vertices.data);

    GfxRenderPassDesc pass_desc{};
    GfxSetColorAttachment(&pass_desc, 0, GfxGetSwapchainTexture());

    auto pass = GfxBeginRenderPass("UI", ctx->cmd_buffer, pass_desc);
    {
        GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});
        GfxSetPipelineState(&pass, &g_ui_pipeline);

        GfxSetVertexBuffer(&pass, Default_Vertex_Buffer_Index, FrameDataBuffer(), vertices_offset, vertices.count * sizeof(UIVertex), sizeof(UIVertex));

        auto vertex_frame_info = GfxGetVertexStageBinding(&g_ui_pipeline, "frame_info_buffer");
        auto fragment_texture = GfxGetFragmentStageBinding(&g_ui_pipeline, "ui_texture");

        GfxSetBuffer(&pass, vertex_frame_info, FrameDataBuffer(), ctx->frame_info_offset, sizeof(Std140FrameInfo));
        GfxSetSamplerState(&pass, fragment_texture, &g_ui_texture_sampler);

        // @Todo @Speed: batch draw calls
        foreach (i, g_ui_elements)
        {
            auto elem = g_ui_elements[i];

            GfxSetTexture(&pass, fragment_texture, elem.texture ? elem.texture : &g_ui_white_texture);

            GfxDrawPrimitives(&pass, 6, 1, i * 6);
        }
    }
    GfxEndRenderPass(&pass);
}
