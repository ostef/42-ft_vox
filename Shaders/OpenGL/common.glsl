#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float3x3 mat3
#define float4x4 mat4
#define uint2 uvec2

#define Pi 3.14159265359
#define Degs_To_Rads (Pi / 180.0)
#define Rads_To_Degs (180.0 / Pi)

#define lerp mix

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

struct SkyAtmosphere
{
    uint2 transmittance_LUT_resolution;
    uint2 multi_scatter_LUT_resolution;
    uint2 color_LUT_resolution;

    int num_transmittance_steps;
    int num_multi_scatter_steps;
    int num_sky_view_scattering_steps;

    float3 rayleigh_scattering_coeff;
    float rayleigh_absorption_coeff;

    float mie_scattering_coeff;
    float mie_absorption_coeff;
    float mie_asymmetry_value;

    float3 ozone_absorption_coeff;

    float3 ground_albedo;
    float ground_level;
    float ground_radius;
    float atmosphere_radius;
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
    SkyAtmosphere sky;
};

const float2 Screen_Space_Position[6] = float2[](
    float2(0,0), float2(1,0), float2(1,1),
    float2(0,0), float2(1,1), float2(0,1)
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

#define QuadCorner uint
#define QuadCorner_TopLeft     0
#define QuadCorner_TopRight    1
#define QuadCorner_BottomLeft  2
#define QuadCorner_BottomRight 3

const float2 Block_Tex_Coords[4] = float2[](
    float2(0,0),
    float2(1,0),
    float2(0,1),
    float2(1,1)
);

#define Block uint

float Acos(float x)
{
    return acos(clamp(x, -1.0, 1.0));
}

float Asin(float x)
{
    return asin(clamp(x, -1.0, 1.0));
}


#define ApplyToneMapping ApplyACESToneMapping

float3 ApplyReinhardToneMapping(float3 color)
{
    return color / (color + float3(1.0));
}

float3 ApplyJodieReinhardToneMapping(float3 color)
{
    // From: https://www.shadertoy.com/view/tdSXzD
    float l = dot(color, float3(0.2126, 0.7152, 0.0722));
    float3 tc = color / (color + 1);

    return lerp(color / (l + 1), tc, tc);
}

float3 ApplyACESToneMapping(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0, 1);
}

float3 LinearTosRGB(float3 color)
{
    return pow(color, float3(1.0 / 2.2));
}

float3 sRGBToLinear(float3 color)
{
    return pow(color, float3(2.2));
}

float InverseLerp(float a, float b, float t)
{
    return (t - a) / (b - a);
}

float3 BilinearLerp(float3 a, float3 b, float3 c, float3 d, float s, float t)
{
    float3 x = lerp(a, b, s);
    float3 y = lerp(c, d, s);

    return lerp(x, y, t);
}
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

bool ApproxZero(float x, float epsilon)
{
    return abs(x) < epsilon;
}

bool ApproxEquals(float a, float b, float epsilon)
{
    return abs(a - b) < epsilon;
}

float LinearRGBToLuminance(float3 rgb)
{
    return dot(clamp(rgb, float3(0), float3(1)), float3(0.2126729, 0.7151522, 0.0721750));
}

float RayIntersectsSphere(float3 ray_origin, float3 ray_direction, float radius)
{
    float b = dot(ray_origin, ray_direction);
    float c = dot(ray_origin, ray_origin) - radius * radius;
    if (c > 0 && b > 0)
        return -1.0;

    float delta = b * b - c;
    if (delta < 0)
        return -1.0;

    // Inside sphere, use far delta
    if (delta > b * b)
        return -b + sqrt(delta);

    return -b - sqrt(delta);
}

float LinearizeDepth(float x, float znear, float zfar)
{
    return znear * zfar / (zfar + x * (znear - zfar));
}
