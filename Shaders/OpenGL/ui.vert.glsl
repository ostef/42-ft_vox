#version 460

#include "common.glsl"

layout(location=0) in vec2 v_position;
layout(location=1) in vec2 v_tex_coords;
layout(location=2) in vec4 v_color;

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec2 tex_coords;
out vec4 color;

void main()
{
    tex_coords = v_tex_coords;
    color = v_color;

    vec2 position = v_position / frame_info.window_pixel_size;
    position = position * 2 - vec2(1);
    gl_Position = vec4(position, 0, 1);
}
