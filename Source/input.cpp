#include "Input.hpp"

static u32 g_prev_mouse_state;
static u32 g_mouse_state;
static u8 g_prev_keyboard_state[256];
static u8 g_keyboard_state[256];
static Vec2f g_mouse_delta;

bool IsMouseButtonDown(MouseButton button)
{
    return (g_mouse_state & SDL_BUTTON((int)button)) != 0;
}

bool IsMouseButtonPressed(MouseButton button)
{
    return (g_prev_mouse_state & SDL_BUTTON((int)button)) == 0
        && (g_mouse_state & SDL_BUTTON((int)button)) != 0;
}

bool IsMouseButtonReleased(MouseButton button)
{
    return (g_prev_mouse_state & SDL_BUTTON((int)button)) != 0
        && (g_mouse_state & SDL_BUTTON((int)button)) == 0;
}

Vec2f GetMouseDelta()
{
    return g_mouse_delta;
}

Vec2f GetMousePosition()
{
    int x, y;
    SDL_GetMouseState(&x, &y);

    return {(float)x, (float)y};
}

bool IsKeyDown(SDL_Scancode scancode)
{
    return g_keyboard_state[scancode];
}

bool IsKeyPressed(SDL_Scancode scancode)
{
    return !g_prev_keyboard_state[scancode] && g_keyboard_state[scancode];
}

bool IsKeyReleased(SDL_Scancode scancode)
{
    return g_prev_keyboard_state[scancode] && !g_keyboard_state[scancode];
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

    g_prev_mouse_state = g_mouse_state;
    g_mouse_state = SDL_GetMouseState(null, null);

    memcpy(g_prev_keyboard_state, g_keyboard_state, sizeof(g_keyboard_state));
    auto kb_state = SDL_GetKeyboardState(null);
    memcpy(g_keyboard_state, kb_state, sizeof(g_keyboard_state));
}
