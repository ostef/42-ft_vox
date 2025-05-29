#version 460

#include "common.glsl"

uniform sampler2D ui_texture;

in float2 tex_coords;
in float4 color;

out float4 frag_color;

void main()
{
    frag_color = texture(ui_texture, tex_coords) * color;
}


