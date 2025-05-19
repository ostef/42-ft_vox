#include "Graphics.hpp"

GfxContext g_gfx_context{};

void GfxCreateContext(SDL_Window *window)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    g_gfx_context.window = window;

    g_gfx_context.gl_context = SDL_GL_CreateContext(window);
    Assert(g_gfx_context.gl_context != null, "Could not create OpenGL graphics context");

    SDL_GL_MakeCurrent(window, g_gfx_context.gl_context);

    gladLoadGLLoader(SDL_GL_GetProcAddress);

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
}

void GfxDestroyContext()
{
}

void GfxBeginFrame()
{
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GfxSubmitFrame()
{
    SDL_GL_SwapWindow(g_gfx_context.window);
}