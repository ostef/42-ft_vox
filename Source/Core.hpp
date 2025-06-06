#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <pthread.h>
#include <semaphore.h>

#if defined(_WIN32)
#define VOX_PLATFORM_WINDOWS
#elif defined(__linux__)
#define VOX_PLATFORM_LINUX
#define VOX_PLATFORM_POSIX
#elif defined(__MACH__)
#define VOX_PLATFORM_MACOS
#define VOX_PLATFORM_POSIX
#else
#error "Unsupported platform"
#endif

typedef  uint8_t  u8;
typedef   int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint64_t u64;
typedef  int64_t s64;
#ifndef uint
typedef unsigned int uint;
#endif

typedef float  f32;
typedef double f64;

#define null nullptr

static char g_assert_failure_message_buffer[10000]; // @Todo @ThreadSafety

#if defined(VOX_PLATFORM_WINDOWS)
#define DebugBreak() __debugbreak()
#elif defined(VOX_PLATFORM_LINUX)
#define DebugBreak() asm volatile("int3")
#elif defined(VOX_PLATFORM_MACOS)
#define DebugBreak() __asm__ volatile(".inst 0xe7f001f0")
#endif

#define Panic(...) do { \
    snprintf(g_assert_failure_message_buffer, 10000, "" __VA_ARGS__); \
    HandlePanic(__FILE__, __LINE__, g_assert_failure_message_buffer);\
} while (0)

#define Assert(x, ...) do { if (!(x)) {\
    snprintf(g_assert_failure_message_buffer, 10000, "" __VA_ARGS__); \
    HandleAssertionFailure(__FILE__, __LINE__, #x, g_assert_failure_message_buffer); }\
} while (0)

static inline void HandlePanic(const char *file, int line, const char *message)
{
    printf("\x1b[1;35mPanic!\x1b[0m At file %s:%d\n%s\n", file, line, message);
    DebugBreak();
}

static inline void HandleAssertionFailure(const char *file, int line, const char *expr, const char *message)
{
    printf("\x1b[1;35mAssertion failed!\x1b[0m At file %s:%d\n%s (%s)\n", file, line, message, expr);
    DebugBreak();
}

extern u64 g_frame_index;

enum AllocatorOp
{
    AllocatorOp_Alloc,
    AllocatorOp_Free,
};

typedef void *(*AllocatorFunc)(AllocatorOp op, s64 size, void *ptr, void *data);

void *HeapAllocator(AllocatorOp op, s64 size, void *ptr, void *data);

struct Allocator
{
    void *data = null;
    AllocatorFunc func = null;
};

extern Allocator heap;
extern Allocator temp;

void *Alloc(s64 size, Allocator allocator);
void Free(void *ptr, Allocator allocator);

template<typename T>
T *Alloc(Allocator allocator)
{
    T *ptr = (T *)Alloc(sizeof(T), allocator);
    *ptr = T{};

    return ptr;
}

template<typename T>
T *Alloc(s64 count, Allocator allocator, bool initialize = false)
{
    T *ptr = (T *)Alloc(sizeof(T) * count, allocator);
    if (initialize)
    {
        for (s64 i = 0; i < count; i += 1)
        {
            ptr[i] = T{};
        }
    }

    return ptr;
}

struct MemoryArenaPage
{
    struct MemoryArenaPage *prev = null;
    s64 used = 0;
    s64 total_size = 0;
};

struct MemoryArena
{
    MemoryArenaPage *last_page = null;
    s64 used = 0;
    s64 total_size = 0;
};

void ResetMemoryArena(MemoryArena *arena);
void *AllocFromArena(MemoryArena *arena, s64 size);

void *MemoryArenaAllocator(AllocatorOp op, s64 size, void *ptr, void *data);

struct String
{
    s64 length = 0;
    char *data = null;

    String(const char *str = "")
    {
        this->length = strlen(str);
        this->data = (char *)str;
    }

    explicit String(s64 length, char *data)
    {
        this->length = length;
        this->data = data;
    }

    inline char &operator [](s64 index)
    {
        Assert(index >= 0 && index < length,
            "Array bounds check failed (attempted index is %lld, length is %lld)",
            index, length
        );

        return data[index];
    }

    inline const char &operator [](s64 index) const
    {
        Assert(index >= 0 && index < length,
            "Array bounds check failed (attempted index is %lld, length is %lld)",
            index, length
        );

        return data[index];
    }
};

#define FSTR(str) (int)((str).length), (str).data

String JoinStrings(String a, String b, String separator, Allocator allocator);
String CloneString(String str, Allocator allocator);
char *CloneToCString(String str, Allocator allocator);
String SPrintf(const char *fmt, ...);
String TPrintf(const char *fmt, ...);

bool Equals(const String &a, const String &b);

inline bool operator ==(const String &a, const String &b) { return Equals(a, b); }

template<typename T, typename Status = bool>
struct Result
{
    T value = {};
    union
    {
        Status status = {};
        Status ok;
    };

    static Result<T, Status> Good(const T &value, Status status)
    {
        Result<T, Status> res = {};
        res.value = value;
        res.status = status;

        return res;
    }

    static Result<T, Status> Bad(Status status)
    {
        Result<T, Status> res = {};
        res.status = status;

        return res;
    }
};

// File stuff

String GetParentDirectory(String filename);
String GetAbsoluteFilename(String filename, Allocator allocator);

bool FileExists(String filename);
Result<String> ReadEntireFile(String filename);

static const char *Log_Graphics = "Graphics";
static const char *Log_Vulkan   = "Graphics/Vulkan";
static const char *Log_OpenGL   = "Graphics/OpenGL";
static const char *Log_Metal    = "Graphics/Metal";
static const char *Log_Shaders  = "Graphics/Shaders";

void LogMessage(const char *section, const char *str, ...);
void LogWarning(const char *section, const char *str, ...);
void LogError(const char *section, const char *str, ...);

// Defer

template<typename T> struct VOX_RemoveRef { using type = T; };
template<typename T> struct VOX_RemoveRef<T &> { using type = T; };
template<typename T> struct VOX_RemoveRef<T &&> { using type = T; };
template<typename T> inline T &&VOX_forward(typename VOX_RemoveRef<T>::type &t) { return static_cast<T &&>(t); }
template<typename T> inline T &&VOX_forward(typename VOX_RemoveRef<T>::type &&t) { return static_cast<T &&>(t); }

template<typename Tproc>
struct Defer
{
	Tproc proc;

	Defer(Tproc &&proc) : proc(VOX_forward<Tproc>(proc)) {}
	~Defer() { proc(); }
};

template<typename Tproc> Defer<Tproc> DeferProcedureCall(Tproc proc) { return Defer<Tproc>(VOX_forward<Tproc>(proc)); }

#define _defer1(x, y) x##y
#define _defer2(x, y) _defer1(x, y)
#define _defer3(x)    _defer2(x, __COUNTER__)
#define defer(code)   auto _defer3(_defer_) = DeferProcedureCall([&]() { code; })

#define StaticArraySize(arr)(sizeof(arr) / sizeof(*(arr)))

#define foreach(it_index, arr) for (s64 it_index = 0; it_index < (arr).count; it_index += 1)

template<typename T>
struct Slice
{
    s64 count = 0;
    T *data = null;

    inline T &operator [](s64 index)
    {
        Assert(index >= 0 && index < count,
            "Array bounds check failed (attempted index is %lld, count is %lld)",
            index, count
        );

        return data[index];
    }

    inline const T &operator [](s64 index) const
    {
        Assert(index >= 0 && index < count,
            "Array bounds check failed (attempted index is %lld, count is %lld)",
            index, count
        );

        return data[index];
    }
};

template<typename T>
Slice<T> AllocSlice(s64 count, Allocator allocator, bool initialize = false)
{
    Slice<T> result{};
    result.data = Alloc<T>(count, allocator, initialize);
    if (result.data != null)
        result.count = count;

    return result;
}

template<typename T>
struct Array
{
    s64 count = 0;
    T *data = null;
    s64 allocated = 0;
    Allocator allocator = {};

    inline T &operator [](s64 index)
    {
        Assert(index >= 0 && index < count,
            "Array bounds check failed (attempted index is %lld, count is %lld)",
            index, count
        );

        return data[index];
    }

    inline const T &operator [](s64 index) const
    {
        Assert(index >= 0 && index < count,
            "Array bounds check failed (attempted index is %lld, count is %lld)",
            index, count
        );

        return data[index];
    }
};

template<typename T>
Slice<T> MakeSlice(Array<T> arr)
{
    return Slice<T>{.count=arr.count, .data=arr.data};
}

template<typename T>
void ArrayReserve(Array<T> *arr, s64 capacity)
{
    if (arr->allocated >= capacity)
        return;

    T *new_data =Alloc<T>(capacity, arr->allocator);
    Assert(new_data != null);

    memcpy(new_data, arr->data, arr->allocated * sizeof(T));

    Free(arr->data, arr->allocator);

    arr->data = new_data;
    arr->allocated = capacity;
}

template<typename T>
void ArrayFree(Array<T> *arr)
{
    Free(arr->data, arr->allocator);
    arr->data = null;
    arr->count = 0;
    arr->allocated = 0;
}

template<typename T>
T *ArrayPush(Array<T> *arr)
{
    if (arr->count >= arr->allocated)
        ArrayReserve(arr, arr->allocated * 2 + 8);

    T *ptr = &arr->data[arr->count];
    *ptr = T{};

    arr->count += 1;

    return ptr;
}

template<typename T>
T *ArrayPush(Array<T> *arr, const T item)
{
    T *ptr = ArrayPush(arr);
    *ptr = item;

    return ptr;
}

template<typename T>
void ArrayPop(Array<T> *arr)
{
    Assert(arr->count > 0);

    arr->count -= 1;
}

template<typename T>
void ArrayClear(Array<T> *arr)
{
    arr->count = 0;
}

template<typename T>
void ArrayOrderedRemoveAt(Array<T> *arr, s64 index)
{
    Assert(index >= 0 && index < arr->count, "Array bounds check failed (got %lld, max is %lld)", index, arr->count);

    for (s64 i = index; i < arr->count - 1; i += 1)
        arr->data[i] = arr->data[i + 1];

    arr->count -= 1;
}

#define Fnv_64_Prime        0x100000001b3
#define Fnv_64_Offset_Basis 0xcbf29ce484222325

static inline u64 Fnv1aHash(u64 val, u64 h = Fnv_64_Offset_Basis)
{
    h ^= val;
    return h * Fnv_64_Prime;
}

static inline u64 Fnv1aHash(void *data, s64 size, u64 h = Fnv_64_Offset_Basis)
{
    for (s64 i = 0; i < size; i += 1)
        h = Fnv1aHash(((u8*)data)[i], h);

    return h;
}

#define Hash_Map_Never_Occupied 0
#define Hash_Map_Removed 1
#define Hash_Map_First_Occupied 2
#define Hash_Map_Min_Capacity 32
#define Hash_Map_Load_Limit 70

template<typename TKey, typename TValue>
struct HashMap
{
    struct Entry
    {
        u64 hash = 0;
        TKey key;
        TValue value;
    };

    typedef bool (*CompareFunc)(TKey a, TKey b);
    typedef u64 (*HashFunc)(TKey key);

    CompareFunc Compare = null;
    HashFunc Hash = null;

    Slice<Entry> entries = {};
    Allocator allocator = {};
    s64 occupied = 0;
    s64 count = 0;
};

template<typename TKey, typename TValue>
void HashMapFree(HashMap<TKey, TValue> *map)
{
    Free(map->entries.data, map->allocator);
    map->entries = {};
    map->occupied = 0;
    map->count = 0;
}

template<typename TKey, typename TValue>
void HashMapGrow(HashMap<TKey, TValue> *map)
{
    typedef typename HashMap<TKey, TValue>::Entry Entry;

    if (!map->allocator.func)
        map->allocator = heap;

    Slice<Entry> old_entries = map->entries;
    defer(Free(old_entries.data, map->allocator));

    s64 new_capacity = (map->entries.count * 2 > Hash_Map_Min_Capacity ? map->entries.count * 2 : Hash_Map_Min_Capacity);
    map->entries = {};
    map->entries.data = Alloc<Entry>(new_capacity, map->allocator);
    map->entries.count = new_capacity;
    map->count = 0;
    map->occupied = 0;

    for (s64 i = 0; i < new_capacity; i += 1)
        map->entries[i].hash = Hash_Map_Never_Occupied;

    for (s64 i = 0; i < old_entries.count; i += 1)
    {
        Entry entry = old_entries[i];
        if (entry.hash >= Hash_Map_First_Occupied)
            HashMapInsert(map, entry.key, entry.value);
    }
}

struct HashMapProbeResult
{
    u64 hash = 0;
    s64 index = -1;
    bool is_present = false;
};

template<typename TKey, typename TValue>
HashMapProbeResult HashMapProbe(HashMap<TKey, TValue> *map, TKey key)
{
    Assert(map->entries.count > 0);
    Assert(map->Compare != null);
    Assert(map->Hash != null);

    u64 mask = (u64)(map->entries.count - 1);
    u64 hash = map->Hash(key);
    if (hash < Hash_Map_First_Occupied)
        hash += Hash_Map_First_Occupied;

    u64 index = hash & mask;
    u64 increment = 1 + (hash >> 27);
    while (map->entries[index].hash != Hash_Map_Never_Occupied)
    {
        auto entry = &map->entries[index];

        if (entry->hash == hash && map->Compare(entry->key, key))
            return {.hash=hash, .index=(s64)index, .is_present=true};

        index += increment;
        index &= mask;
        increment += 1;
    }

    return {.hash=hash, .index=(s64)index, .is_present=false};
}

template<typename TKey, typename TValue>
TValue *HashMapFindOrAdd(HashMap<TKey, TValue> *map, TKey key, bool *was_present = null)
{
    if ((map->occupied + 1) * 100 >= map->entries.count * Hash_Map_Load_Limit)
        HashMapGrow(map);

    auto probe = HashMapProbe(map, key);
    auto entry = &map->entries[probe.index];
    if (was_present)
        *was_present = probe.is_present;

    if (probe.is_present)
        return &entry->value;

    entry->hash = probe.hash;
    entry->key = key;
    map->occupied += 1;
    map->count += 1;

    return &entry->value;
}

template<typename TKey, typename TValue>
void HashMapInsert(HashMap<TKey, TValue> *map, TKey key, TValue value)
{
    auto ptr = HashMapFindOrAdd(map, key);
    *ptr = value;
}

template<typename TKey, typename TValue>
TValue *HashMapFindPtr(HashMap<TKey, TValue> *map, TKey key)
{
    if (map->count <= 0)
        return null;

    auto probe = HashMapProbe(map, key);
    if (probe.is_present)
        return &map->entries[probe.index].value;

    return null;
}

template<typename TKey, typename TValue>
TValue HashMapFind(HashMap<TKey, TValue> *map, TKey key, TValue fallback = {})
{
    auto ptr = HashMapFindPtr(map, key);
    if (!ptr)
        return fallback;

    return *ptr;
}

template<typename TKey, typename TValue>
bool HashMapRemove(HashMap<TKey, TValue> *map, TKey key, TValue *removed_value = null)
{
    if (map->count <= 0)
    {
        if (removed_value)
            *removed_value = {};

        return false;
    }

    auto probe = HashMapProbe(map, key);
    if (!probe.is_present)
    {
        if (removed_value)
            *removed_value = {};

        return false;
    }

    auto entry = &map->entries[probe.index];
    entry->hash = Hash_Map_Removed;
    map->count -= 1;

    if (removed_value)
        *removed_value = entry->value;

    return true;
}

// Inspired by Jai's Thread module

typedef void (*ThreadGroupFunc)(struct ThreadGroup *, void *work);

struct ThreadWorkEntry
{
    ThreadWorkEntry *next = null;
    void *work = null;
    int worker_thread_index = -1;
};

struct ThreadWorkList
{
    pthread_mutex_t mutex = {};
    sem_t semaphore = {}; // When signaled, this means "hey something was added!"
    ThreadWorkEntry *first = null;
    ThreadWorkEntry *last = null;
    int count = 0;
};

struct WorkerThread
{
    struct ThreadGroup *group = null;
    pthread_t thread = 0;
    ThreadWorkList available_work = {};
    ThreadWorkList completed_work = {};
};

struct ThreadGroup
{
    String name = "";
    ThreadGroupFunc func = null;
    Slice<WorkerThread> worker_threads = {};
    int worker_thread_assign_index = 0;

    bool initialized = false;
    bool started = false;
    bool should_stop = false;
};

void InitThreadGroup(ThreadGroup *group, String name, ThreadGroupFunc func, int num_threads);
void DestroyThreadGroup(ThreadGroup *group);
void Start(ThreadGroup *group);
void Stop(ThreadGroup *group);
void AddWork(ThreadGroup *group, void *work);
Slice<void *> GetCompletedWork(ThreadGroup *group);

float GetTimeInSeconds();

struct Settings
{
    int render_distance = 25;
};

extern Settings g_settings;