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
    Vec2f position;
    Vec2f size;
    GfxTexture *texture;
    Vec4f color;
    Vec2f uv0, uv1;
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
    String text = id;
    for (int i = 0; i < text.length; i += 1)
    {
        if (text[i] == '#')
        {
            text.length = i;
            break;
        }
    }

    float padding = UI_Button_Padding;
    Vec2f size = CalculateTextSize(text) + Vec2f{2 * padding, 2 * padding};

    UIRectElement bg{};
    bg.size = size;
    bg.position = LayoutElem(size);
    bg.color = {0, 0, 0, 1};

    Vec2f mouse_pos = GetMousePosition();
    bool hovered = g_ui_has_mouse
        && mouse_pos.x > bg.position.x && mouse_pos.x < bg.position.x + bg.size.x
        && mouse_pos.y > bg.position.y && mouse_pos.y < bg.position.y + bg.size.y;

    if (hovered)
        bg.color = {0.4, 0.4, 0.4, 1};

    ArrayPush(&g_ui_elements, bg);

    UITextAt(bg.position + Vec2f{padding, padding}, text);

    return hovered && IsMouseButtonReleased(MouseButton_Left);
}

bool UINoiseParams(String id, NoiseParams *params)
{
    NoiseParams old = *params;
    if (UIButton("-#scale"))
        params->scale -= IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.01 : 0.1;
    UISameLine();
    if (UIButton("+#scale"))
        params->scale += IsKeyDown(SDL_SCANCODE_LSHIFT) ? 0.01 : 0.1;
    UISameLine();
    UIText(TPrintf("scale: %.3f", params->scale));

    if (UIButton("-#octaves"))
        params->octaves -= 1;
    UISameLine();
    if (UIButton("+#octaves"))
        params->octaves += 1;
    params->octaves = Clamp(params->octaves, 1, Perlin_Fractal_Max_Octaves);
    UISameLine();
    UIText(TPrintf("octaves: %d", params->octaves));

    params->max_amplitude = PerlinFractalMax(params->octaves, params->persistance);

    return memcmp(&old, params, sizeof(NoiseParams)) != 0;
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
