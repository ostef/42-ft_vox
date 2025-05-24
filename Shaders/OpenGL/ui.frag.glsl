#version 460

#include "common.glsl"

uniform sampler2D ui_texture;

in vec2 tex_coords;
in vec4 color;

out vec4 frag_color;

void main()
{
    frag_color = texture(ui_texture, tex_coords) * color;
}


