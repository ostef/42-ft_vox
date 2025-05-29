#version 460

#include "common.glsl"
#include "shadow.glsl"

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

layout(binding=0) uniform sampler2DArray block_atlas;

layout(binding=1) uniform sampler2DArrayShadow shadow_map;
layout(binding=2) uniform sampler2DArray shadow_map_noise;

flat in Block block;
flat in BlockFace block_face;
in vec2 screen_position;
in vec3 position;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

void main()
{
    float light_intensity = max(dot(normal, -frame_info.sun_direction), 0);
    light_intensity *= 1 - SampleShadowMap(frame_info.shadow_map, shadow_map_noise, shadow_map, frame_info.sun_direction, position, normal, screen_position);

    vec3 base_color = texture(block_atlas, vec3(tex_coords, block_face)).rgb;
    vec3 diffuse = base_color * light_intensity;
    vec3 ambient = base_color * 0.3;
    vec3 color = ambient + diffuse;

    frag_color = vec4(color,1);
}
