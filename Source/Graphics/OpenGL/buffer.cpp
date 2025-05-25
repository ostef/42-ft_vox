#include "Graphics.hpp"

bool IsNull(GfxBuffer *buffer)
{
    return buffer == null || buffer->handle == 0;
}

GfxBufferDesc GetDesc(GfxBuffer *buffer)
{
    return buffer->desc;
}

GfxBuffer GfxCreateBuffer(String name, GfxBufferDesc desc)
{
    Assert(desc.size > 0);

    GLuint handle = 0;
    glCreateBuffers(1, &handle);

    String buffer_name = TPrintf("Buffer '%.*s'", FSTR(name));
    glObjectLabel(GL_BUFFER, handle, buffer_name.length, buffer_name.data);

    GLenum access_flags = 0;
    if (desc.cpu_access & GfxCpuAccess_Write)
        access_flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
    if (desc.cpu_access & GfxCpuAccess_Read)
        access_flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;

    glNamedBufferStorage(handle, desc.size, null, access_flags);

    return {
        .handle=handle,
        .desc=desc,
    };
}

void GfxDestroyBuffer(GfxBuffer *buffer)
{
    glDeleteBuffers(1, &buffer->handle);
    *buffer = {};
}

void *GfxMapBuffer(GfxBuffer *buffer, s64 offset, s64 size, GfxMapAccessFlags access)
{
    Assert(offset >= 0 && size >= 0);

    GLenum flags = GL_MAP_PERSISTENT_BIT;
    if (access & GfxMapAccess_Write)
    {
        flags |= GL_MAP_WRITE_BIT;
        flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    if (access & GfxMapAccess_Read)
    {
        flags |= GL_MAP_READ_BIT;
    }

    return glMapNamedBufferRange(buffer->handle, offset, size, flags);
}

void GfxUnmapBuffer(GfxBuffer *buffer)
{
    glUnmapNamedBuffer(buffer->handle);
}

void GfxFlushMappedBuffer(GfxBuffer *buffer, s64 offset, s64 size)
{
    Assert(offset >= 0 && size >= 0);

    glFlushMappedNamedBufferRange(buffer->handle, offset, size);
}
