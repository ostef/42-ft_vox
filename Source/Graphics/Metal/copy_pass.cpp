#include "Graphics.hpp"

GfxCopyPass GfxBeginCopyPass(String name, GfxCommandBuffer *cmd_buffer) { return {}; }
void GfxEndCopyPass(GfxCopyPass *pass) {}

void GfxGenerateMipmaps(GfxCopyPass *pass, GfxTexture *texture) {}

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
) {}

void GfxCopyTextureToBuffer(
    GfxCopyPass *pass,
    GfxTexture *src_texture,
    Vec3u src_origin,
    Vec3u src_size,
    GfxBuffer *dst_buffer,
    u64 dst_offset,
    u32 src_slice,
    u32 src_level
) {}

void GfxCopyBufferToBuffer(
    GfxCopyPass *pass,
    GfxBuffer *src_buffer,
    s64 src_offset,
    GfxBuffer *dst_buffer,
    s64 dst_offset,
    s64 size
) {}
