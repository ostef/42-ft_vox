// WARNING: must match the structs in Renderer.hpp
struct Camera
{
    float fov_in_degrees;
    float z_near_dist;
    float z_far_dist;
    mat4 transform;
    mat4 view;
    mat4 projection;
};

#define Shadow_Map_Num_Cascades 4

struct ShadowMap
{
    int resolution;
    int noise_resolution;
    vec2 depth_bias_min_max;
    float normal_bias;
    float filter_radius;
    mat4 cascade_matrices[Shadow_Map_Num_Cascades];
    float cascade_sizes[Shadow_Map_Num_Cascades];
};

struct FrameInfo
{
    vec2 window_pixel_size;
    float window_scale_factor;
    vec3 sun_direction;
    Camera camera;
    vec2 texture_atlas_size;
    vec2 texture_block_size;
    ShadowMap shadow_map;
};

const vec2 Screen_Space_Position[6] = vec2[](
    vec2(0,0), vec2(1,1), vec2(0,1),
    vec2(0,0), vec2(1,0), vec2(1,1)
);

#define BlockFace uint
#define BlockFace_East   0
#define BlockFace_West   1
#define BlockFace_Top    2
#define BlockFace_Bottom 3
#define BlockFace_North  4
#define BlockFace_South  5

const vec3 Block_Normals[6] = vec3[](
    vec3( 1, 0, 0),
    vec3(-1, 0, 0),
    vec3( 0, 1, 0),
    vec3( 0,-1, 0),
    vec3( 0, 0, 1),
    vec3( 0, 0,-1)
);

#define BlockCorner uint
#define BlockCorner_TopLeft     0
#define BlockCorner_TopRight    1
#define BlockCorner_BottomLeft  2
#define BlockCorner_BottomRight 3

const vec2 Block_Tex_Coords[4] = vec2[](
    vec2(0,0),
    vec2(1,0),
    vec2(0,1),
    vec2(1,1)
);

#define Block uint

float Random(float seed)
{
    return fract(sin(seed * 91.3458) * 47453.5453);
}

vec3 RandomColor(float seed)
{
    vec3 result;
    result.r = Random(seed);
    result.g = Random(result.r);
    result.b = Random(result.g);

    return result;
}
