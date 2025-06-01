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

enum BlockCorner
{
    BlockCorner_EastNorthTop,
    BlockCorner_EastNorthBottom,
    BlockCorner_EastSouthTop,
    BlockCorner_EastSouthBottom,
    BlockCorner_WestNorthTop,
    BlockCorner_WestNorthBottom,
    BlockCorner_WestSouthTop,
    BlockCorner_WestSouthBottom,
};

typedef u32 BlockCornerFlags;
#define BlockCornerFlag_EastNorthTop    (1 << (u32)BlockCorner_EastNorthTop)
#define BlockCornerFlag_EastNorthBottom (1 << (u32)BlockCorner_EastNorthBottom)
#define BlockCornerFlag_EastSouthTop    (1 << (u32)BlockCorner_EastSouthTop)
#define BlockCornerFlag_EastSouthBottom (1 << (u32)BlockCorner_EastSouthBottom)
#define BlockCornerFlag_WestNorthTop    (1 << (u32)BlockCorner_WestNorthTop)
#define BlockCornerFlag_WestNorthBottom (1 << (u32)BlockCorner_WestNorthBottom)
#define BlockCornerFlag_WestSouthTop    (1 << (u32)BlockCorner_WestSouthTop)
#define BlockCornerFlag_WestSouthBottom (1 << (u32)BlockCorner_WestSouthBottom)

static const Vec3f Block_Normals[6] = {
    { 1, 0, 0},
    {-1, 0, 0},
    { 0, 1, 0},
    { 0,-1, 0},
    { 0, 0, 1},
    { 0, 0,-1},
};
static const Vec3f Block_Tangents[6] = {
    { 0, 0, 1},
    { 0, 0,-1},
    { 1, 0, 0},
    { 1, 0, 0},
    {-1, 0, 0},
    { 1, 0, 0},
};
static const Vec3f Block_Bitangents[6] = {
    { 0, 1, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 0, 0,-1},
    { 0, 1, 0},
    { 0, 1, 0},
};
static const Vec3f Block_Face_Start[6] = {
    { 1, 0, 0},
    { 0, 0, 1},
    { 0, 1, 0},
    { 0, 0, 0},
    { 1, 0, 1},
    { 0, 0, 0},
};

struct BlockVertex
{
    Vec3f position;
    float block_height;
    uint block;
    uint block_face;
    uint block_corner;
    uint occlusion;
};

Slice<GfxVertexInputDesc> MakeBlockVertexLayout();

enum ChunkMeshType : s8
{
    ChunkMeshType_Air = -1,
    ChunkMeshType_Solid = 0,
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

extern bool g_show_debug_atlas;

struct Std140FrameInfo;

struct FrameRenderContext
{
    GfxCommandBuffer *cmd_buffer = null;
    Std140FrameInfo *frame_info = null;
    s64 frame_info_offset = -1;
    World *world = null;
};

extern GfxTexture g_main_color_texture;
extern GfxTexture g_main_depth_texture;
extern GfxSamplerState g_linear_sampler;
extern GfxSamplerState g_block_sampler;

void InitRenderer();
void RenderGraphics(World *world);

#define Block_Texture_Size_No_Border 16
#define Block_Texture_Border 4
#define Block_Texture_Size (Block_Texture_Size_No_Border + Block_Texture_Border * 2)
#define Block_Atlas_Num_Blocks 16
#define Block_Atlas_Size (Block_Texture_Size * Block_Atlas_Num_Blocks)

extern GfxTexture g_block_atlas;
extern GfxTexture g_debug_block_face_atlas;

void LoadAllTextures();
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

struct SkyAtmosphere
{
    Vec2u transmittance_LUT_resolution = {256, 64};
    Vec2u multi_scatter_LUT_resolution = {32, 32};
    Vec2u color_LUT_resolution = {200, 200};

    int num_transmittance_steps = 40;
    int num_multi_scatter_steps = 20;
    int num_color_scattering_steps = 32;

    // Rayleigh theory represents the behavior of light when interacting
    // with air molecules. We assume that light is never absorbed and
    // can only scatter around
    Vec3f rayleigh_scattering_coeff = {5.802, 13.558, 33.1};
    float rayleigh_absorption_coeff = 0;

    // Mie theory represents the behavior of light when interacting with
    // aerosols such as dust or pollution. Light can be scattered or absorbed
    float mie_scattering_coeff = 3.996;
    float mie_absorption_coeff = 4.4;
    // Between -1 and 1, > 0 light is scattered forward,
    // < 0 light is scattered backwards
    float mie_asymmetry_value = 0.8;

    // Ozone is a specific component of the Earth that has been identified
    // as important for representing its atmosphere, since it is key to
    // achieving sky-blue colors when the sun is at the horizon. Ozone does
    // not contribute to scattering; it only absorbs light
    Vec3f ozone_absorption_coeff = {0.650, 1.881, 0.085};

    Vec3f ground_albedo = {0.3, 0.3, 0.3};
    float ground_level = 6.3602;
    float ground_radius = 6.360;
    float atmosphere_radius = 6.460;

    GfxTexture transmittance_LUT = {};
    GfxTexture multi_scatter_LUT = {};
    GfxTexture color_LUT = {};
};

extern SkyAtmosphere g_sky;

void RenderSkyLUTs(FrameRenderContext *ctx);
void SkyAtmospherePass(FrameRenderContext *ctx);

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

struct Std140SkyAtmosphere
{
    Vec2u transmittance_LUT_resolution;
    Vec2u multi_scatter_LUT_resolution;
    Vec2u color_LUT_resolution;
    int num_transmittance_steps;
    int num_multi_scatter_steps;
    int num_color_scattering_steps;
    u32 _padding0[3] = {0};
    Vec3f rayleigh_scattering_coeff;
    float rayleigh_absorption_coeff;
    float mie_scattering_coeff;
    float mie_absorption_coeff;
    float mie_asymmetry_value;
    u32 _padding1 = 0;
    Vec3f ozone_absorption_coeff;
    u32 _padding2 = 0;
    Vec3f ground_albedo;
    float ground_level;
    float ground_radius;
    float atmosphere_radius;
    u32 _padding3[2] = {0};
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
    Std140SkyAtmosphere sky;
};

#pragma pack(pop)
