#include "Core.hpp"
#include "Math.hpp"

#ifdef VOX_PLATFORM_LINUX
#include <unistd.h>
#endif


void *HeapAllocator(AllocatorOp op, s64 size, void *ptr, void *data)
{
    switch (op)
    {
    case AllocatorOp_Alloc: return malloc(size);
    case AllocatorOp_Free:  free(ptr); break;
    }

    return null;
}

void *Alloc(s64 size, Allocator allocator)
{
    Assert(allocator.func != null, "Allocator is null");

    if (size <= 0)
        return null;

    return allocator.func(AllocatorOp_Alloc, size, null, allocator.data);
}

void Free(void *ptr, Allocator allocator)
{
    Assert(allocator.func != null, "Allocator is null");

    if (ptr == null)
        return;

    allocator.func(AllocatorOp_Free, 0, ptr, allocator.data);
}

static void AddPage(MemoryArena *arena, s64 min_size)
{
    min_size = Max(min_size, 4096 - (s64)sizeof(MemoryArenaPage));

    MemoryArenaPage *page = (MemoryArenaPage *)malloc(min_size + sizeof(MemoryArenaPage));
    *page = {};

    page->total_size = min_size;
    arena->total_size += min_size;

    page->prev = arena->last_page;
    arena->last_page = page;
}

void ResetMemoryArena(MemoryArena *arena)
{
    while (arena->last_page)
    {
        MemoryArenaPage *prev = arena->last_page->prev;
        free(arena->last_page);

        arena->last_page = prev;
    }
}

void *AllocFromArena(MemoryArena *arena, s64 size)
{
    MemoryArenaPage *page = arena->last_page;

    if (!page || page->used + size > page->total_size)
    {
        AddPage(arena, size);
        page = arena->last_page;
    }

    void *ptr = (char *)(page + 1) + page->used;
    page->used += size;

    return ptr;
}

void *MemoryArenaAllocator(AllocatorOp op, s64 size, void *ptr, void *data)
{
    switch (op)
    {
    case AllocatorOp_Alloc: return AllocFromArena((MemoryArena *)data, size);
    case AllocatorOp_Free:  break; // No-op
    }

    return null;
}

void LogMessage(const char *section, const char *str, ...)
{
    if (section)
        printf("[%s] ", section);

    va_list args;
    va_start(args, str);

    vprintf(str, args);

    va_end(args);

    printf("\n");
}

void LogWarning(const char *section, const char *str, ...)
{
    printf("\x1b[1;33m");

    if (section)
        printf("[%s] ", section);

    printf("Warning: ");

    va_list args;
    va_start(args, str);

    vprintf(str, args);

    va_end(args);

    printf("\x1b[0m\n");
}

void LogError(const char *section, const char *str, ...)
{
    printf("\x1b[1;31m");

    if (section)
        printf("[%s] ", section);

    printf("Error: ");

    va_list args;
    va_start(args, str);

    vprintf(str, args);

    va_end(args);

    printf("\x1b[0m\n");
}

bool FileExists(String filename)
{
    char *c_filename = CloneToCString(filename, temp);

    return access(c_filename, F_OK) != -1;
}

Result<String> ReadEntireFile(String filename)
{
    char *c_filename = CloneToCString(filename, temp);
    FILE *file = fopen(c_filename, "rb");
    if (!file)
        return Result<String>::Bad(false);

    defer(fclose(file));

    fseek(file, 0, SEEK_END);
    s64 size = ftell(file);
    rewind(file);

    char *data =(char *)malloc(size + 1);
    if (!data)
        return Result<String>::Bad(false);

    s64 number_of_bytes_read = fread(data, 1, size, file);
    data[number_of_bytes_read] = 0;

    String str = String{number_of_bytes_read, data};

    return Result<String>::Good(str, true);
}

bool Equals(const String &a, const String &b)
{
    if (a.length != b.length)
        return false;

    return strncmp(a.data, b.data, a.length) == 0;
}

char *CloneToCString(String str, Allocator allocator)
{
    char *result = Alloc<char>(str.length + 1, allocator);
    memcpy(result, str.data, str.length);
    result[str.length] = 0;

    return result;
}

String SPrintf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int length = vsnprintf(null, 0, fmt, args);
    va_end(args);

    if (length < 0)
        return {};

    char *buffer = Alloc<char>(length + 1, heap);

    va_start(args, fmt);
    length = vsnprintf(buffer, length + 1, fmt, args);
    va_end(args);

    return String{length, buffer};
}

String TPrintf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int length = vsnprintf(null, 0, fmt, args);
    va_end(args);

    if (length < 0)
        return {};

    char *buffer = Alloc<char>(length + 1, temp);

    va_start(args, fmt);
    length = vsnprintf(buffer, length + 1, fmt, args);
    va_end(args);

    return String{length, buffer};
}
