#pragma once

#include "Core.hpp"
#include "Math.hpp"
#include "Graphics.hpp"
#include "Renderer.hpp"

void UIBeginFrame();
void UIRender(FrameRenderContext *ctx);

void UIImage(float x, float y, float w, float h, GfxTexture *texture, Vec4f color = {1,1,1,1}, Vec2f uv0 = {0,0}, Vec2f uv1 = {1,1});
