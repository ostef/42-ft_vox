#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float3x3 mat3
#define float4x4 mat4

#define Pi 3.14159265359

// WARNING: must match the structs in Renderer.hpp
struct Camera
{
    float fov_in_degrees;
    float z_near_dist;
    float z_far_dist;
    float3 position;
    float3 right;
    float3 up;
    float3 direction;
    float4x4 transform;
    float4x4 view;
    float4x4 projection;
};

#define Shadow_Map_Num_Cascades 4

struct ShadowMap
{
    int resolution;
    int noise_resolution;
    float2 depth_bias_min_max;
    float normal_bias;
    float filter_radius;
    float4x4 cascade_matrices[Shadow_Map_Num_Cascades];
    float cascade_sizes[Shadow_Map_Num_Cascades];
};

struct FrameInfo
{
    float2 window_pixel_size;
    float window_scale_factor;
    float3 sun_direction;
    float4 sun_color;
    Camera camera;
    float2 texture_atlas_size;
    float2 texture_block_size;
    float2 texture_border_size;
    ShadowMap shadow_map;
};

const float2 Screen_Space_Position[6] = float2[](
    float2(0,0), float2(1,1), float2(0,1),
    float2(0,0), float2(1,0), float2(1,1)
);

#define BlockFace uint
#define BlockFace_East   0
#define BlockFace_West   1
#define BlockFace_Top    2
#define BlockFace_Bottom 3
#define BlockFace_North  4
#define BlockFace_South  5

const float3 Block_Normals[6] = float3[](
    float3( 1, 0, 0),
    float3(-1, 0, 0),
    float3( 0, 1, 0),
    float3( 0,-1, 0),
    float3( 0, 0, 1),
    float3( 0, 0,-1)
);

#define BlockCorner uint
#define BlockCorner_TopLeft     0
#define BlockCorner_TopRight    1
#define BlockCorner_BottomLeft  2
#define BlockCorner_BottomRight 3

const float2 Block_Tex_Coords[4] = float2[](
    float2(0,0),
    float2(1,0),
    float2(0,1),
    float2(1,1)
);

#define Block uint

float Random(float seed)
{
    return fract(sin(seed * 91.3458) * 47453.5453);
}

float3 RandomColor(float seed)
{
    float3 result;
    result.r = Random(seed);
    result.g = Random(result.r);
    result.b = Random(result.g);

    return result;
}
