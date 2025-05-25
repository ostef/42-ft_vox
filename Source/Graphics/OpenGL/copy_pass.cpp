#include "Graphics.hpp"

GfxCopyPass GfxBeginCopyPass(String name, GfxCommandBuffer *cmd_buffer)
{
    GfxBeginDebugGroup(cmd_buffer, name);

    return {
        .name=name,
        .cmd_buffer=cmd_buffer,
    };
}

void GfxEndCopyPass(GfxCopyPass *pass)
{
    GfxEndDebugGroup(pass->cmd_buffer);
}

void GfxGenerateMipmaps(GfxCopyPass *pass, GfxTexture *texture)
{
    glGenerateTextureMipmap(texture->handle);
}

void GfxCopyTextureToTexture(
    GfxCopyPass *pass,
    GfxTexture *src_texture,
    Vec3u src_origin,
    Vec3u src_size,
    GfxTexture *dst_texture,
    Vec3u dst_origin,
    u32 src_slice,
    u32 src_level,
    u32 dst_slice,
    u32 dst_level
)
{
    GfxTextureType src_type = GetDesc(src_texture).type;
    GfxTextureType dst_type = GetDesc(dst_texture).type;

    // OpenGL does not make a difference between array slice and texture 3rd dimension
    // src_depth := ifx src_type == .TEXTURE_3D
    //     then src_origin.z
    //     else src_slice;
    // dst_depth := ifx dst_type == .TEXTURE_3D
    //     then dst_origin.z
    //     else dst_slice;

    u32 src_depth = src_slice;
    u32 dst_depth = dst_slice;

    glCopyImageSubData(
        src_texture->handle,
        GLTextureType(src_type),
        src_level,
        src_origin.x, src_origin.y, src_depth,
        dst_texture->handle,
        GLTextureType(dst_type),
        dst_level,
        dst_origin.x, dst_origin.y, dst_depth,
        src_size.x, src_size.y, src_size.z
    );
}

void GfxCopyTextureToBuffer(
    GfxCopyPass *pass,
    GfxTexture *src_texture,
    Vec3u src_origin,
    Vec3u src_size,
    GfxBuffer *dst_buffer,
    u64 dst_offset,
    u32 src_slice,
    u32 src_level
)
{
    // @Todo: handle depth/slice/level
    glBindBuffer(GL_PIXEL_PACK_BUFFER, dst_buffer->handle);

    GfxPixelFormat pixel_format = GetDesc(src_texture).pixel_format;
    GLenum format, type;
    GLPixelFormatAndType(pixel_format, &format, &type);

    glGetTextureImage(
        // xx src_origin.x, xx src_origin.y,
        // xx src_size.x, xx src_size.y,
        src_texture->handle,
        src_level,
        format, type,
        GetDesc(dst_buffer).size,
        (void *)dst_offset
    );

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void GfxCopyBufferToBuffer(
    GfxCopyPass *pass,
    GfxBuffer *src_buffer,
    s64 src_offset,
    GfxBuffer *dst_buffer,
    s64 dst_offset,
    s64 size
)
{
    glCopyNamedBufferSubData(src_buffer->handle, dst_buffer->handle, src_offset, dst_offset, size);
}
