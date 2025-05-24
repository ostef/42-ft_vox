#include "UI.hpp"
#include "Renderer.hpp"

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

static const char UI_Font_Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()[]{}<>/*:#%!?.,'\"@&$";

static Array<UIRectElement> g_ui_elements;
static GfxTexture g_ui_font;

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

    ArrayClear(&g_ui_elements);
}

static void PushRect(Array<UIVertex> *vertices, UIRectElement elem)
{
    auto v = ArrayPush(vertices);
    v->position = elem.position;
    v->tex_coords = {elem.uv0.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = elem.position + Vec2f{elem.size.x, elem.size.y};
    v->tex_coords = {elem.uv1.x, elem.uv1.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = elem.position + Vec2f{0, elem.size.y};
    v->tex_coords = {elem.uv0.x, elem.uv1.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = elem.position;
    v->tex_coords = {elem.uv0.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = elem.position + Vec2f{elem.size.x, 0};
    v->tex_coords = {elem.uv1.x, elem.uv0.y};
    v->color = elem.color;

    v = ArrayPush(vertices);
    v->position = elem.position + Vec2f{elem.size.x, elem.size.y};
    v->tex_coords = {elem.uv1.x, elem.uv1.y};
    v->color = elem.color;
}

static GfxPipelineState g_ui_pipeline;
static GfxSamplerState g_ui_texture_sampler;

static void InitPipeline()
{
    GfxPipelineStateDesc desc{};
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
        PushRect(&vertices, g_ui_elements[i]);

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

            GfxSetTexture(&pass, fragment_texture, elem.texture);

            GfxDrawPrimitives(&pass, 6, 1, i * 6);
        }
    }
    GfxEndRenderPass(&pass);
}

void UIImage(float x, float y, float w, float h, GfxTexture *texture, Vec2f uv0, Vec2f uv1)
{
    UIRectElement elem = {
        .position = {x, y},
        .size = {w, h},
        .texture = texture,
        .color = {1,1,1,1},
        .uv0 = uv0,
        .uv1 = uv1,
    };

    ArrayPush(&g_ui_elements, elem);
}

void UIText(float x, float y, String text, float scale)
{
    float x_origin = x;
    float y_origin = y;

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
            elem.uv0.y = (char_index / num_chars_x + 1) / (float)num_chars_y;
            elem.uv1.x = elem.uv0.x + 1 / (float)num_chars_x;
            elem.uv1.y = elem.uv0.y - 1 / (float)num_chars_y;

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
