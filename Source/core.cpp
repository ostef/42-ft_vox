#include "Core.hpp"
#include "Math.hpp"

#if defined(VOX_PLATFORM_POSIX)
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

String GetParentDirectory(String filename)
{
    s64 slash_index = filename.length;
    if (filename[filename.length - 1] == '/')
        slash_index -= 1;

    while (slash_index > 0 && filename[slash_index - 1] != '/')
        slash_index -= 1;

    filename.length = slash_index;

    return filename;
}

String GetAbsoluteFilename(String filename)
{
    Panic("@Todo");
    return filename;
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

String JoinStrings(String a, String b, String separator, Allocator allocator)
{
    String result;
    result.length = a.length + b.length + separator.length;
    result.data = Alloc<char>(result.length, allocator);

    memcpy(result.data, a.data, a.length);
    memcpy(result.data + a.length, separator.data, separator.length);
    memcpy(result.data + a.length + separator.length, b.data, b.length);

    return result;
}

String CloneString(String str, Allocator allocator)
{
    String result{};
    result.data = Alloc<char>(str.length, allocator);
    if (result.data)
    {
        result.length = str.length;
        memcpy(result.data, str.data, str.length);
    }

    return result;
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

static void *WorkerThreadRoutine(void *data);

static void InitWorkList(ThreadWorkList *list)
{
    pthread_mutex_init(&list->mutex, null);
    sem_init(&list->semaphore, 0, 0);
}

static void DestroyWorkList(ThreadWorkList *list)
{
    pthread_mutex_destroy(&list->mutex);
    sem_destroy(&list->semaphore);
}

void InitThreadGroup(ThreadGroup *group, String name, ThreadGroupFunc func, int num_threads)
{
    Assert(num_threads > 1);
    Assert(func != null);

    group->name = name;
    group->func = func;
    group->worker_threads = AllocSlice<WorkerThread>(num_threads, heap, true);

    foreach (i, group->worker_threads)
    {
        auto worker = &group->worker_threads[i];
        worker->group = group;

        InitWorkList(&worker->available_work);
        InitWorkList(&worker->completed_work);
    }

    group->initialized = true;
}

void DestroyThreadGroup(ThreadGroup *group)
{
    if (group->started)
        Stop(group);

    foreach (i, group->worker_threads)
    {
        auto worker = &group->worker_threads[i];

        DestroyWorkList(&worker->available_work);
        DestroyWorkList(&worker->completed_work);
    }

    Free(group->worker_threads.data, heap);
    *group = {};
}

void Start(ThreadGroup *group)
{
    Assert(group->initialized, "Thread group is not initialized");
    Assert(!group->started, "Thread group has already been started");

    foreach (i, group->worker_threads)
    {
        auto worker = &group->worker_threads[i];
        int status = pthread_create(&worker->thread, null, WorkerThreadRoutine, worker);
        Assert(status == 0);
    }

    group->started = true;
}

void Stop(ThreadGroup *group)
{
    Assert(group->started, "Thread group has not been started");

    group->should_stop = true;

    foreach (i, group->worker_threads)
    {
        auto worker = &group->worker_threads[i];
        sem_post(&worker->available_work.semaphore);

        pthread_join(worker->thread, null);
        worker->thread = 0;
    }

    group->started = false;
}

static void AddWorkToList(ThreadWorkList *list, ThreadWorkEntry *entry)
{
    pthread_mutex_lock(&list->mutex);

    if (list->last)
        list->last->next = entry;
    else
        list->first = entry;

    list->last = entry;
    list->count += 1;

    pthread_mutex_unlock(&list->mutex);

    sem_post(&list->semaphore);
}

static ThreadWorkEntry *GetWorkFromList(ThreadWorkList *list)
{
    pthread_mutex_lock(&list->mutex);

    auto result = list->first;

    list->first = result->next;
    if (!list->first)
        list->last = null;

    list->count -= 1;

    result->next = null;

    pthread_mutex_unlock(&list->mutex);

    return result;
}

void AddWork(ThreadGroup *group, void *work)
{
    Assert(group->started, "Thread group has not been started");

    if (group->should_stop)
        return;

    auto entry = Alloc<ThreadWorkEntry>(heap);
    entry->work = work;

    entry->worker_thread_index = group->worker_thread_assign_index;

    group->worker_thread_assign_index += 1;
    if (group->worker_thread_assign_index >= group->worker_threads.count)
        group->worker_thread_assign_index = 0;

    AddWorkToList(&group->worker_threads[entry->worker_thread_index].available_work, entry);
}

Slice<void *> GetCompletedWork(ThreadGroup *group)
{
    Array<void *> result = {.allocator=temp};

    ThreadWorkEntry *completed = null;
    int count = 0;
    foreach (i, group->worker_threads)
    {
        auto worker = &group->worker_threads[i];

        pthread_mutex_lock(&worker->completed_work.mutex);

        count += worker->completed_work.count;

        if (worker->completed_work.last)
        {
            worker->completed_work.last->next = completed;
            completed = worker->completed_work.first;
        }

        worker->completed_work.first = null;
        worker->completed_work.last = null;
        worker->completed_work.count = 0;

        pthread_mutex_unlock(&worker->completed_work.mutex);
    }

    while (completed)
    {
        auto next = completed->next;

        ArrayPush(&result, completed->work);

        Free(completed, heap);

        completed = next;
    }

    Assert(result.count == count);

    return MakeSlice(result);
}

void *WorkerThreadRoutine(void *data)
{
    auto worker = (WorkerThread *)data;

    while (!worker->group->should_stop)
    {
        sem_wait(&worker->available_work.semaphore);
        if (worker->group->should_stop)
            break;

        auto entry = GetWorkFromList(&worker->available_work);

        if (entry)
        {
            worker->group->func(worker->group, entry->work);
            AddWorkToList(&worker->completed_work, entry);
        }
    }

    return null;
}
