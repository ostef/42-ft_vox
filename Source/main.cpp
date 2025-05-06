#include "Core.hpp"

static MemoryArena g_frame_arena;

static Allocator g_heap = Allocator{null, HeapAllocator};
static Allocator g_temp = Allocator{&g_frame_arena, MemoryArenaAllocator};

Allocator *heap = &g_heap;
Allocator *temp = &g_temp;

int main(int argc, char **args)
{
    while(true)
    {
        ResetMemoryArena(&g_frame_arena);
    }
}
