#version 450

#include "common.glsl"

uniform sampler2D my_texture;

out vec4 frag_color;

void main()
{
    frag_color = texture(my_texture, gl_FragCoord.xy);
}
