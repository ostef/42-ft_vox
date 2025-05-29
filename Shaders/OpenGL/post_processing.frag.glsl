#version 460

#include "common.glsl"

uniform sampler2D main_texture;

in float2 position;

out float4 frag_color;

void main()
{
    frag_color = texture(main_texture, position);
}
