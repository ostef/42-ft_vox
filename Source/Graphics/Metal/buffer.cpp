#include "Graphics.hpp"

bool IsNull(GfxBuffer *buffer)
{
    return buffer == null || buffer->handle == null;
}

GfxBufferDesc GetDesc(GfxBuffer *buffer)
{
    return buffer->desc;
}

GfxBuffer GfxCreateBuffer(String name, GfxBufferDesc desc) { return {}; }
void GfxDestroyBuffer(GfxBuffer *buffer) {}

void *GfxMapBuffer(GfxBuffer *buffer, s64 offset, s64 size, GfxMapAccessFlags access) { return null; }
void GfxUnmapBuffer(GfxBuffer *buffer) {}
void GfxFlushMappedBuffer(GfxBuffer *buffer, s64 offset, s64 size) {}
