#version 460

#include "sky.glsl"

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

layout(binding=0) uniform sampler2D transmittance_LUT;
layout(binding=1) uniform sampler2D color_LUT;

in float2 position;

out float4 frag_color;

void main()
{
    float cam_width_scale = 2 * tan(frame_info.camera.fov_in_degrees * Degs_To_Rads * 0.5);
    float cam_height_scale = cam_width_scale * frame_info.window_pixel_size.y / frame_info.window_pixel_size.x;

    float2 xy = 2 * position - float2(1);
    float3 ray_direction = normalize(
        frame_info.camera.direction
        + frame_info.camera.right * xy.x * cam_width_scale
        + frame_info.camera.up * xy.y * cam_height_scale
    );

    float3 color = GetColor(frame_info.sky, transmittance_LUT, color_LUT, -frame_info.sun_direction, ray_direction);

    frag_color = float4(color, 1);
}
