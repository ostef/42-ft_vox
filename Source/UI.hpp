#pragma once

#include "Core.hpp"
#include "Math.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"

void UIBeginFrame();
void UIRender(FrameRenderContext *ctx);

void UISetCursorStartX(float x);
void UISameLine();
void UIImage(GfxTexture *texture, Vec2f size, Vec2f uv0 = {0,0}, Vec2f uv1 = {1,1});
void UITextAt(Vec2f position, String text);
void UIText(String text);
bool UIButton(String id);
