#pragma once

#include "Core.hpp"
#include "Math.hpp"

#include <SDL.h>

enum MouseButton
{
    MouseButton_Left = 1,
    MouseButton_Middle = 2,
    MouseButton_Right = 3,
};

bool IsMouseButtonDown(MouseButton button);
bool IsKeyDown(SDL_Scancode scancode);
Vec2f GetMouseDelta();

void HandleInputEvent(SDL_Event event);
void UpdateInput();
