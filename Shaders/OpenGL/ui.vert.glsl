#version 460

#include "common.glsl"

layout(location=0) in float2 v_position;
layout(location=1) in float2 v_tex_coords;
layout(location=2) in float4 v_color;

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

out gl_PerVertex
{
    float4 gl_Position;
};

out float2 tex_coords;
out float4 color;

void main()
{
    tex_coords = v_tex_coords;
    color = v_color;

    float2 position = v_position / frame_info.window_pixel_size;
    position = position * 2 - float2(1);
    gl_Position = float4(position, 0, 1);
}
