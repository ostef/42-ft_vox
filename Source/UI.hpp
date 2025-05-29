#pragma once

#include "Core.hpp"
#include "Math.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"

void UIBeginFrame();
void UIRender(FrameRenderContext *ctx);

void UISetMouse(bool has_mouse);
void UISetCursorStart(float x, float y);
void UISameLine();
void UIImage(GfxTexture *texture, Vec2f size, Vec2f uv0 = {0,0}, Vec2f uv1 = {1,1});
void UITextAt(Vec2f position, String text);
void UIText(String text);
bool UIButton(String id);
bool UIFloatEdit(String id, float *value, float min, float max);
bool UIIntEdit(String id, int *value, int min, int max);
bool UINoiseParams(String id, NoiseParams *params);
bool UISplineEditor(String id, Spline *spline, Vec2f size);
