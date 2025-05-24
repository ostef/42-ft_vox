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
bool IsMouseButtonPressed(MouseButton button);
bool IsMouseButtonReleased(MouseButton button);

Vec2f GetMouseDelta();

bool IsKeyDown(SDL_Scancode scancode);
bool IsKeyPressed(SDL_Scancode scancode);
bool IsKeyReleased(SDL_Scancode scancode);

void HandleInputEvent(SDL_Event event);
void UpdateInput();
