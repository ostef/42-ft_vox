#include "UI.hpp"
#include "Renderer.hpp"

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

static Array<UIRectElement> g_ui_elements;

void UIBeginFrame()
{
    if (!g_ui_elements.allocator.func)
        g_ui_elements.allocator = heap;

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
    sampler_desc.min_filter = GfxSamplerFilter_Linear;
    sampler_desc.mag_filter = GfxSamplerFilter_Linear;
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

void UIImage(float x, float y, float w, float h, GfxTexture *texture, Vec4f color, Vec2f uv0, Vec2f uv1)
{
    UIRectElement elem = {
        .position = {x, y},
        .size = {w, h},
        .texture = texture,
        .color = color,
        .uv0 = uv0,
        .uv1 = uv1,
    };

    ArrayPush(&g_ui_elements, elem);
}
