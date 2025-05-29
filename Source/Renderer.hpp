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

enum BlockFace : uint
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

enum BlockCorner
{
    BlockCorner_TopLeft,
    BlockCorner_TopRight,
    BlockCorner_BottomLeft,
    BlockCorner_BottomRight,
};

struct BlockVertex
{
    Vec3f position;
    float block_height;
    int block;
    int block_face;
    int block_corner;
};

struct Mesh
{
    GfxBuffer vertex_buffer = {};
    GfxBuffer index_buffer = {};
    u32 vertex_count = 0;
    u32 index_count = 0;
    bool uploaded = false;
};

void GenerateChunkMeshWorker(ThreadGroup *group, void *data);
void CancelChunkMeshUpload(Chunk *chunk);

struct FrameRenderContext
{
    GfxCommandBuffer *cmd_buffer = null;
    s64 frame_info_offset = -1;
};

void InitRenderer();
void RenderGraphics(World *world);

#define Block_Texture_Size 16
#define Block_Atlas_Num_Blocks 16
#define Block_Atlas_Size (Block_Texture_Size * Block_Atlas_Num_Blocks)

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
    Vec2f texture_atlas_size;
    Vec2f texture_block_size;
};

#pragma pack(pop)
