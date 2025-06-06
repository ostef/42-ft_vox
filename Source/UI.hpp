#pragma once

#include "Core.hpp"
#include "Math.hpp"
#include "Graphics.hpp"
#include "Graphics/Renderer.hpp"

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

extern Array<UIRectElement> g_ui_elements;
extern GfxTexture g_ui_font;
extern GfxTexture g_ui_white_texture;

void UIBeginFrame();
void UIRenderPass(FrameRenderContext *ctx);

void UISetMouse(bool has_mouse);
void UISetCursorStart(float x, float y);
void UISameLine();
void UIImage(GfxTexture *texture, Vec2f size, Vec2f uv0 = {0,0}, Vec2f uv1 = {1,1});
void UITextAt(Vec2f position, String text);
void UIText(String text);
bool UIButton(String id);
bool UICheckbox(String id, bool *value);
bool UIFloatEdit(String id, float *value, float min, float max, float step = 1);
bool UIIntEdit(String id, int *value, int min, int max, int step = 1);
bool UINoiseParams(String id, NoiseParams *params);
bool UISplineEditor(String id, Spline *spline, Vec2f size, float min_x = INFINITY, float max_x = INFINITY, float min_y = INFINITY, float max_y = INFINITY, float step_x = 0.1, float step_y = 0.1);

void UpdateUI(World *world);
