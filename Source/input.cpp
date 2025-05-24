#include "Input.hpp"

static const u8 *g_keyboard_state;
static Vec2f g_mouse_delta;

bool IsMouseButtonDown(MouseButton button)
{
    return (SDL_GetMouseState(null, null) & SDL_BUTTON((int)button)) != 0;
}

bool IsKeyDown(SDL_Scancode scancode)
{
    if (!g_keyboard_state)
        g_keyboard_state = SDL_GetKeyboardState(null);

    return g_keyboard_state[scancode];
}

Vec2f GetMouseDelta()
{
    return g_mouse_delta;
}

void HandleInputEvent(SDL_Event event)
{
    switch (event.type)
    {
    case SDL_MOUSEMOTION: {
        g_mouse_delta.x += (float)event.motion.xrel;
        g_mouse_delta.y += (float)event.motion.yrel;
    } break;
    }
}

void UpdateInput()
{
    g_mouse_delta = {};
}
