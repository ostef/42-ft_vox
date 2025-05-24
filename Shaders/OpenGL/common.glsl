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

struct FrameInfo
{
    vec2 window_pixel_size;
    float window_scale_factor;
    Camera camera;
    vec2 texture_atlas_size;
    vec2 texture_block_size;
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
