#version 460

#include "common.glsl"
#include "fxaa.glsl"

uniform sampler2D main_texture;

in float2 position;

out float4 frag_color;

void main()
{
    float3 color = FXAA(main_texture, position, 1 / vec2(textureSize(main_texture, 0))).rgb;
    frag_color = vec4(color, 1);
}
