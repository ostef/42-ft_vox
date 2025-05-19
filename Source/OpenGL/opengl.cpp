#include "Graphics.hpp"

GfxContext g_gfx_context{};

static void GLDebugMessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *user_param
)
{
    const char *source_str = "";
    switch (source)
    {
    case GL_DEBUG_SOURCE_API: source_str = "API "; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_str = "window system "; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "shader compiler "; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: source_str = "third party "; break;
    case GL_DEBUG_SOURCE_APPLICATION: source_str = "application "; break;
    }

    const char *type_str = "";
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: type_str = ""; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = ", deprecated behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = ", undefined behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY: type_str = ", portability concern"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: type_str = ", performance concern"; break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        LogError(Log_OpenGL, "%serror%s: (%s) %s", source_str, type_str, id, message);

        #ifdef Gfx_Break_On_Error
            Panic("Hit an OpenGL error", location=location);
        #endif

        break;

    case GL_DEBUG_SEVERITY_MEDIUM:
    case GL_DEBUG_SEVERITY_LOW:
        LogWarning(Log_OpenGL, "%swarning%s: (%s) %s", source_str, type_str, id, message);

        break;
    }
}

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

    s32 major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    Assert(major == 4 && minor == 6, "Could not load OpenGL 4.6");

    LogMessage(Log_OpenGL, "Initialized OpenGL 4.6 context");
    LogMessage(Log_OpenGL, "OpenGL renderer: %s", glGetString(GL_RENDERER));

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugMessageCallback, null);

    g_gfx_context.dummy_swapchain_texture.handle = 0xffffffff;
    g_gfx_context.dummy_swapchain_texture.desc.type = GfxTextureType_Texture2D;
    g_gfx_context.dummy_swapchain_texture.desc.pixel_format = GfxGetSwapchainPixelFormat();
    g_gfx_context.dummy_swapchain_texture.desc.usage = GfxTextureUsage_RenderTarget;

    glGenQueries(StaticArraySize(g_gfx_context.frame_time_queries), g_gfx_context.frame_time_queries);

    // Make first query result not cause an error
    for (int backbuffer_index = 0; backbuffer_index < Gfx_Max_Frames_In_Flight; backbuffer_index += 1)
    {
        for (int i = 0; i < 2; i += 1)
        {
            glQueryCounter(g_gfx_context.frame_time_queries[backbuffer_index * 2 + i], GL_TIMESTAMP);

            char name[100];
            sprintf(name, "Query 'Frame Time %s, Backbuffer %d'", (i == 0 ? "Start" : "End"), backbuffer_index);

            glObjectLabel(GL_QUERY, g_gfx_context.frame_time_queries[backbuffer_index * 2 + i], -1, name);
        }
    }

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
}

void GfxDestroyContext()
{
    glDeleteQueries(StaticArraySize(g_gfx_context.frame_time_queries), g_gfx_context.frame_time_queries);

    SDL_GL_DeleteContext(g_gfx_context.gl_context);
}

void GfxBeginFrame()
{
    int w, h;
    SDL_GetWindowSizeInPixels(g_gfx_context.window, &w, &h);

    g_gfx_context.dummy_swapchain_texture.desc.width = (u32)w;
    g_gfx_context.dummy_swapchain_texture.desc.height = (u32)h;

    int last_query_index = g_gfx_context.backbuffer_index - (Gfx_Max_Frames_In_Flight - 1);
    if (last_query_index < 0)
        last_query_index += Gfx_Max_Frames_In_Flight;

    s64 start_time, end_time;
    glGetQueryObjecti64v(g_gfx_context.frame_time_queries[last_query_index * 2 + 0], GL_QUERY_RESULT, &start_time);
    glGetQueryObjecti64v(g_gfx_context.frame_time_queries[last_query_index * 2 + 1], GL_QUERY_RESULT, &end_time);

    s64 elapsed = end_time - start_time;
    g_gfx_context.last_frame_gpu_time = (float)(elapsed / 1'000'000'000.0);

    glQueryCounter(g_gfx_context.frame_time_queries[g_gfx_context.backbuffer_index * 2 + 0], GL_TIMESTAMP);
}

void GfxSubmitFrame()
{
    glQueryCounter(g_gfx_context.frame_time_queries[g_gfx_context.backbuffer_index * 2 + 1], GL_TIMESTAMP);

    SDL_GL_SwapWindow(g_gfx_context.window);

    g_gfx_context.backbuffer_index = (g_gfx_context.backbuffer_index + 1) % Gfx_Max_Frames_In_Flight;
}

int GfxGetBackbufferIndex()
{
    return g_gfx_context.backbuffer_index;
}

float GfxGetLastFrameGPUTime()
{
    return g_gfx_context.last_frame_gpu_time;
}

GfxTexture *GfxGetSwapchainTexture()
{
    return &g_gfx_context.dummy_swapchain_texture;
}

GfxPixelFormat GfxGetSwapchainPixelFormat()
{
    GLint r_size, g_size, b_size, a_size;
    GLint type;
    GLint encoding;
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &r_size);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &g_size);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &b_size);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &a_size);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);

    if (encoding == GL_LINEAR)
    {
        if (r_size == 8 && g_size == 8 && b_size == 8 && (a_size == 0 || a_size == 8))
        {
            switch (type)
            {
            case GL_FLOAT:
                return GfxPixelFormat_RGBAFloat32;
            // case GL_UNSIGNED_INT:
            //     return GfxPixelFormat_RGBAUint8;
            case GL_UNSIGNED_NORMALIZED:
                return GfxPixelFormat_RGBAUnorm8;
            }
        }
        // else if (r_size == 32 && g_size == 0 && b_size == 0 && a_size == 0)
        // {
        //     switch (type)
        //     {
        //     case GL_FLOAT:
        //         return .RED_FLOAT32;
        //     case GL_UNSIGNED_INT:
        //         return .RGBA_UINT32;
        //     }
        // }
    }
    else if (encoding == GL_SRGB)
    {
        if (r_size == 8 && g_size == 8 && b_size == 8 && (a_size == 0 || a_size == 8))
        {
            switch (type)
            {
            case GL_UNSIGNED_NORMALIZED:
                return GfxPixelFormat_RGBAUnorm8;
            }
        }
    }

    Panic("Invalid parameters for pixel format (RGBA bits: %d %d %d %d, type: %x, encoding: %x)", r_size, g_size, b_size, a_size, type, encoding);

    return GfxPixelFormat_Invalid;
}
