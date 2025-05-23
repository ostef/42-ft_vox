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
};

struct ChunkInfo
{
    mat4 transform;
};

const vec2 Screen_Space_Position[6] = vec2[](
    vec2(0,0), vec2(1,1), vec2(0,1),
    vec2(0,0), vec2(1,0), vec2(1,1)
);
