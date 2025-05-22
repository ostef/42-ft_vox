#include "Graphics.hpp"

bool IsNull(GfxTexture *texture)
{
    return texture == null || texture->handle == 0;
}

GfxTextureDesc GetDesc(GfxTexture *texture)
{
    return texture->desc;
}

GfxTexture GfxCreateTexture(String name, GfxTextureDesc desc)
{
    GLuint handle = 0;
    glCreateTextures(GLTextureType(desc.type), 1, &handle);

    String texture_name = TPrintf("Texture '%.*s'", name.length, name.data);
    glObjectLabel(GL_TEXTURE, handle, texture_name.length, texture_name.data);

    GLenum internal_format = GLPixelFormat(desc.pixel_format);

    // Clear error
    glGetError();

    switch (desc.type)
    {
    default: Panic("Invalid texture type"); break;
    case GfxTextureType_Texture2D:
        glTextureStorage2D(
            handle,
            desc.num_mipmap_levels,
            internal_format,
            desc.width, desc.height
        );
        break;
    }

    // if desc.type != GfxTextureType_Texture2DMultisample
    // {
        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // }

    GLenum err = glGetError();
    if (err != 0)
    {
        LogError(Log_OpenGL, "Could not create texture '%.*s'", name.length, name.data);
        glDeleteTextures(1, &handle);
        return {};
    }

    return {
        .handle=handle,
        .desc=desc,
    };
}

void InvalidateFramebuffersUsingTexture(GLuint handle);

void GfxDestroyTexture(GfxTexture *texture)
{
    InvalidateFramebuffersUsingTexture(texture->handle);
    glDeleteTextures(1, &texture->handle);
    *texture = {};
}

void GfxReplaceTextureRegion(GfxTexture *texture, Vec3u origin, Vec3u size, u32 mipmap_level, u32 array_slice, void *bytes)
{
    auto desc = GetDesc(texture);

    GLenum gl_format, gl_type;
    GLPixelFormatAndType(desc.pixel_format, &gl_format, &gl_type);
    Assert(gl_format != 0 && gl_type != 0);

    switch (desc.type)
    {
    case GfxTextureType_Texture2D:
        glTextureSubImage2D(
            texture->handle,
            mipmap_level,
            origin.x, origin.y,
            size.x, size.y,
            gl_format, gl_type,
            bytes
        );
        break;
    }
}

bool IsNull(GfxSamplerState *sampler)
{
    return sampler == null || sampler->handle == 0;
}

GfxSamplerStateDesc GetDesc(GfxSamplerState *sampler)
{
    return sampler->desc;
}

GfxSamplerState GfxCreateSamplerState(String name, GfxSamplerStateDesc desc)
{
    // Validate();

    GLuint handle = 0;
    glCreateSamplers(1, &handle);

    String sampler_name = TPrintf("Sampler State '%.*s'", name.length, name.data);
    glObjectLabel(GL_SAMPLER, handle, sampler_name.length, sampler_name.data);

    // @Todo: use_average_lod

    glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, GLTextureFilter(desc.min_filter, desc.mip_filter));
    glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, GLTextureFilter(desc.mag_filter, GfxSamplerFilter_None));
    glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, GLTextureWrap(desc.u_address_mode));
    glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, GLTextureWrap(desc.v_address_mode));
    glSamplerParameteri(handle, GL_TEXTURE_WRAP_R, GLTextureWrap(desc.w_address_mode));
    // glSamplerParameterf(handle, GL_TEXTURE_MAX_ANISOTROPY, (float)desc.max_anisotropy);
    glSamplerParameterf(handle, GL_TEXTURE_MIN_LOD, desc.lod_min_clamp);
    glSamplerParameterf(handle, GL_TEXTURE_MAX_LOD, desc.lod_max_clamp);

    if (desc.compare_func != GfxCompareFunc_None)
    {
        glSamplerParameteri(handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(handle, GL_TEXTURE_COMPARE_FUNC, GLComparisonFunc(desc.compare_func));
    }
    else
    {
        glSamplerParameteri(handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    Vec4f border_color = {};
    switch (desc.border_color)
    {
    case GfxSamplerBorderColor_Transparent:
        border_color = {0,0,0,0};
        break;
    case GfxSamplerBorderColor_OpaqueBlack:
        border_color = {0,0,0,1};
        break;
    case GfxSamplerBorderColor_OpaqueWhite:
        border_color = {1,1,1,1};
        break;
    }

    glSamplerParameterfv(handle, GL_TEXTURE_BORDER_COLOR, (float *)&border_color);

    glSamplerParameteri(handle, GL_TEXTURE_CUBE_MAP_SEAMLESS, 1);

    return {
        .handle=handle,
        .desc=desc,
    };
}

void GfxDestroySamplerState(GfxSamplerState *sampler)
{
    glDeleteSamplers(1, &sampler->handle);
    *sampler = {};
}

GLenum GLTextureType(GfxTextureType type)
{
    switch (type)
    {
    case GfxTextureType_Invalid: Panic("Invalid texture type"); break;
    case GfxTextureType_Texture2D: return GL_TEXTURE_2D;
    }

    Panic("Unhandled texture type");
    return 0;
}

GLenum GLPixelFormat(GfxPixelFormat format)
{
    switch (format)
    {
    case GfxPixelFormat_Invalid: Panic("Invalid pixel format"); break;
    case GfxPixelFormat_RGBAUnorm8: return GL_RGBA8;
    case GfxPixelFormat_RGBAFloat32: return GL_RGBA32F;
    case GfxPixelFormat_DepthFloat32: return GL_DEPTH_COMPONENT32F;
    }
    Panic("Unhandled pixel format");
    return 0;
}

void GLPixelFormatAndType(GfxPixelFormat pixel_format, GLenum *format, GLenum *type)
{
    switch (pixel_format)
    {
    case GfxPixelFormat_Invalid: Panic("Invalid pixel format"); break;
    case GfxPixelFormat_DepthFloat32: break;
    case GfxPixelFormat_RGBAUnorm8:  *format = GL_RGBA; *type = GL_UNSIGNED_BYTE; break;
    case GfxPixelFormat_RGBAFloat32: *format = GL_RGBA; *type = GL_FLOAT;         break;
    }

    *format = 0;
    *type = 0;
    Panic("Unhandled pixel format");
    return;
}

GLenum GLTextureFilter(GfxSamplerFilter filter, GfxSamplerFilter mip_filter)
{
    switch (mip_filter)
    {
    case GfxSamplerFilter_None:
        switch (filter)
        {
        case GfxSamplerFilter_None:
            return 0;
        case GfxSamplerFilter_Nearest:
            return GL_NEAREST;
        case GfxSamplerFilter_Linear:
            return GL_LINEAR;
        }
        break;

    case GfxSamplerFilter_Nearest:
        switch (filter)
        {
        case GfxSamplerFilter_None:
            return 0;
        case GfxSamplerFilter_Nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case GfxSamplerFilter_Linear:
            return GL_LINEAR_MIPMAP_NEAREST;
        }
        break;

    case GfxSamplerFilter_Linear:
        switch (filter)
        {
        case GfxSamplerFilter_None:
            return 0;
        case GfxSamplerFilter_Nearest:
            return GL_NEAREST_MIPMAP_LINEAR;
        case GfxSamplerFilter_Linear:
            return GL_LINEAR_MIPMAP_LINEAR;
        }
        break;
    }
    Panic("Invalid filter");
    return 0;
}

GLenum GLTextureWrap(GfxSamplerAddressMode mode)
{
    switch (mode)
    {
    case GfxSamplerAddressMode_ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case GfxSamplerAddressMode_ClampToZero:
        return GL_CLAMP_TO_EDGE;
    case GfxSamplerAddressMode_ClampToBorder:
        return GL_CLAMP_TO_BORDER;
    case GfxSamplerAddressMode_Repeat:
        return GL_REPEAT;
    case GfxSamplerAddressMode_MirrorClampToEdge:
        return GL_MIRROR_CLAMP_TO_EDGE;
    case GfxSamplerAddressMode_MirrorRepeat:
        return GL_MIRRORED_REPEAT;
    }
    Panic("Invalid address mode");
    return 0;
}
