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

enum QuadCorner
{
    QuadCorner_TopLeft,
    QuadCorner_TopRight,
    QuadCorner_BottomLeft,
    QuadCorner_BottomRight,
};

typedef u32 QuadCornerFlags;
#define QuadCornerFlag_TopLeft     (1 << (u32)QuadCorner_TopLeft)
#define QuadCornerFlag_TopRight    (1 << (u32)QuadCorner_TopRight)
#define QuadCornerFlag_BottomLeft  (1 << (u32)QuadCorner_BottomLeft)
#define QuadCornerFlag_BottomRight (1 << (u32)QuadCorner_BottomRight)

struct BlockVertex
{
    Vec3f position;
    float block_height;
    int block;
    int block_face;
    int block_corner;
};

Slice<GfxVertexInputDesc> MakeBlockVertexLayout();

enum ChunkMeshType : u8
{
    ChunkMeshType_Solid,
    ChunkMeshType_Translucent,

    ChunkMeshType_Count,
};

struct Mesh
{
    GfxBuffer vertex_buffer = {};
    GfxBuffer index_buffer = {};
    u32 vertex_count = 0;
    u32 index_count = 0;

    u32 mesh_type_vertex_offsets[ChunkMeshType_Count] = {};
    u32 mesh_type_index_offsets[ChunkMeshType_Count] = {};
    u32 mesh_type_index_counts[ChunkMeshType_Count] = {};

    bool uploaded = false;
};

void GenerateChunkMeshWorker(ThreadGroup *group, void *data);
void CancelChunkMeshUpload(Chunk *chunk);

struct FrameRenderContext
{
    GfxCommandBuffer *cmd_buffer = null;
    s64 frame_info_offset = -1;
    World *world = null;
};

void InitRenderer();
void RenderGraphics(World *world);

#define Block_Texture_Size_No_Border 16
#define Block_Texture_Border 4
#define Block_Texture_Size (Block_Texture_Size_No_Border + Block_Texture_Border * 2)
#define Block_Atlas_Num_Blocks 16
#define Block_Atlas_Size (Block_Texture_Size * Block_Atlas_Num_Blocks)

extern GfxTexture g_block_atlas;

void LoadBlockAtlasTexture();
void QueueGenerateMipmaps(GfxTexture *texture);
void GeneratePendingMipmaps(GfxCommandBuffer *cmd_buffer);

#define Shadow_Map_Default_Resolution 2048
#define Shadow_Map_Num_Cascades 4
#define Shadow_Map_Noise_Size 32
#define Shadow_Map_Num_Filtering_Samples_Sqrt 8
#define Shadow_Map_Num_Filtering_Samples (Shadow_Map_Num_Filtering_Samples_Sqrt * Shadow_Map_Num_Filtering_Samples_Sqrt)

extern GfxTexture g_shadow_map_texture;
extern GfxSamplerState g_shadow_map_sampler;
extern GfxTexture g_shadow_map_noise_texture;
extern GfxSamplerState g_shadow_map_noise_sampler;

extern float g_shadow_map_cascade_sizes[Shadow_Map_Num_Cascades];
extern float g_shadow_map_depth_extent_factor;
extern float g_shadow_map_forward_offset;
extern float g_shadow_map_min_depth_bias;
extern float g_shadow_map_max_depth_bias;
extern float g_shadow_map_normal_bias;
extern float g_shadow_map_filter_radius;

Mat4f GetShadowMapCascadeMatrix(Vec3f light_direction, Mat4f camera_transform, int level);

void InitShadowMap();
void RecreateShadowMapTexture(u32 resolution);
void ShadowMapPass(FrameRenderContext *ctx);

#pragma pack(push, 1)

struct Std140Camera
{
    float fov_in_degrees;
    float z_near_dist;
    float z_far_dist;
    u32 _padding0 = 0;
    Vec3f position;
    u32 _padding1 = 0;
    Vec3f right;
    u32 _padding2 = 0;
    Vec3f up;
    u32 _padding3 = 0;
    Vec3f direction;
    u32 _padding4 = 0;
    Mat4f transform;
    Mat4f view;
    Mat4f projection;
};

struct Std140ShadowMap
{
    int resolution;
    int noise_resolution;
    Vec2f depth_bias_min_max;
    float normal_bias;
    float filter_radius;
    u32 _padding0[2] = {0};
    Mat4f cascade_matrices[Shadow_Map_Num_Cascades];
    Vec4f cascade_sizes[Shadow_Map_Num_Cascades];
};

struct Std140FrameInfo
{
    Vec2f window_pixel_size;
    float window_scale_factor;
    u32 _padding0 = 0;
    Vec3f sun_direction;
    u32 _padding1 = 0;
    Vec4f sun_color;
    Std140Camera camera;
    Vec2f texture_atlas_size;
    Vec2f texture_block_size;
    Vec2f texture_block_border;
    u32 _padding2[2] = {0};
    Std140ShadowMap shadow_map;
};

#pragma pack(pop)
