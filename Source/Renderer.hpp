#pragma once

#include "Core.hpp"
#include "Math.hpp"
#include "Graphics.hpp"

extern SDL_Window *g_window;

#define Default_Vertex_Buffer_Index 10

struct World;
struct Chunk;
enum Block : u8;

struct ShaderFile
{
    String name = "";
    GfxPipelineStageFlags stages = 0;
    GfxShader vertex_shader = {};
    GfxShader fragment_shader = {};
};

void LoadAllShaders();

GfxShader *GetVertexShader(String name);
GfxShader *GetFragmentShader(String name);

struct Lexer
{
    String filename = "";
    String file_contents = "";
    s64 cursor = 0;
};

struct ShaderPreprocessor
{
    Array<String> all_loaded_files = {};
    Array<Lexer> file_stack = {};
    Lexer *current_lexer = null;
    Array<char> string_builder = {};
};

struct ShaderPreprocessResult
{
    Slice<String> all_loaded_files = {};
    String source_code = "";
    bool ok = false;
};

bool InitShaderPreprocessor(ShaderPreprocessor *pp, String filename);
void DestroyShaderPreprocessor(ShaderPreprocessor *pp);
ShaderPreprocessResult PreprocessShader(ShaderPreprocessor *pp);

struct GfxAllocator
{
    GfxBuffer buffer = {};
    void *mapped_ptr = null;
    s64 top = 0;
    s64 capacity = 0;
};

GfxAllocator *FrameDataGfxAllocator();
GfxBuffer *FrameDataBuffer();
Allocator FrameDataAllocator();

void InitGfxAllocator(GfxAllocator *allocator, String name, s64 capacity);
void FlushGfxAllocator(GfxAllocator *allocator);
void ResetGfxAllocator(GfxAllocator *allocator);

s64 GetBufferOffset(GfxAllocator *allocator, void *ptr);

Allocator MakeAllocator(GfxAllocator *allocator);
void *GfxAllocatorFunc(AllocatorOp op, s64 size, void *ptr, void *data);

enum BlockFace : u8
{
    BlockFace_East,
    BlockFace_West,
    BlockFace_Top,
    BlockFace_Bottom,
    BlockFace_North,
    BlockFace_South,
};

typedef u32 BlockFaceFlags;
#define BlockFaceFlag_East   (1 << (u32)BlockFace_East)
#define BlockFaceFlag_West   (1 << (u32)BlockFace_West)
#define BlockFaceFlag_Top    (1 << (u32)BlockFace_Top)
#define BlockFaceFlag_Bottom (1 << (u32)BlockFace_Bottom)
#define BlockFaceFlag_North  (1 << (u32)BlockFace_North)
#define BlockFaceFlag_South  (1 << (u32)BlockFace_South)
#define BlockFaceFlag_All    0x3f

struct BlockVertex
{
    Vec3f position;
    float block_height;
    BlockFace face;
    Block block;
};

struct Mesh
{
    GfxBuffer vertex_buffer = {};
    GfxBuffer index_buffer = {};
    u32 vertex_count = 0;
    u32 index_count = 0;
};

void GenerateChunkMesh(Chunk *chunk);

struct Camera
{
    Vec3f position = {};
    Quatf rotation = {};

    float target_yaw = 0;
    float current_yaw = 0;
    float target_pitch = 0;
    float current_pitch = 0;

    float fov_in_degrees = 80;
    float z_near_dist = 0.1;
    float z_far_dist = 1000.0;

    Mat4f transform = {};
    Mat4f view = {};
    Mat4f projection = {};
};

#pragma pack(push, 1)

struct Std140Camera
{
    float fov_in_degrees;
    float z_near_dist;
    float z_far_dist;
    u32 _padding0[1] = {0};
    Mat4f transform;
    Mat4f view;
    Mat4f projection;
};

struct Std140FrameInfo
{
    Vec2f window_pixel_size;
    float window_scale_factor;
    u32 _padding0[1] = {0};
    Std140Camera camera;
};

struct Std430ChunkInfo
{
    Mat4f transform;
};

#pragma pack(pop)

void UpdateCamera(Camera *camera);
void CalculateCameraMatrices(Camera *camera);

void InitRenderer();
void RenderGraphics(World *world);
