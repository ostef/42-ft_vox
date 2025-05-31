#version 460

#include "common.glsl"
#include "pbr.glsl"
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
in float3 position;
in float3 normal;
in float2 tex_coords;
in float2 face_coords;
in float occlusion;

out float4 frag_color;

void main()
{
    float3 N = normalize(normal);
    float3 V = normalize(frame_info.camera.position - position);
    float3 L = -frame_info.sun_direction;

    float sun_shadow = 1 - SampleShadowMap(frame_info.shadow_map, shadow_map_noise, shadow_map, frame_info.sun_direction, position, normal, gl_FragCoord.xy);
    float3 sun_color = frame_info.sun_color.rgb * frame_info.sun_color.a;

    const float Roughness_Amplitude = 0.4;

    float4 base_color = texture(block_atlas, float3(tex_coords, block_face));
    float roughness = 0.9 - length(base_color.rgb) * Roughness_Amplitude;
    float3 brdf = CalculateBRDF(base_color.rgb, 0, roughness, N, V, L, sun_color * sun_shadow);
    float3 ambient = mix(0.3, 1.0, occlusion) * base_color.rgb * 0.3;
    float3 color = ambient + brdf;
    // color = mix(float3(1,0,0), base_color.rgb, occlusion);

    frag_color = float4(color, base_color.a);
}
