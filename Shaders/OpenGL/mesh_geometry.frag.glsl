#version 460

#include "common.glsl"
#include "pbr.glsl"
#include "shadow.glsl"
#include "Sky/sky.glsl"

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

layout(binding=0) uniform sampler2DArray block_atlas;

layout(binding=1) uniform sampler2DArrayShadow shadow_map;
layout(binding=2) uniform sampler2DArray shadow_map_noise;

layout(binding=3) uniform sampler2D sky_transmittance_LUT;
layout(binding=4) uniform sampler2D sky_color_LUT;

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

    float3 sun_color = GetColorNoToneMapping(frame_info.sky, sky_transmittance_LUT, sky_color_LUT, -frame_info.sun_direction, -frame_info.sun_direction);
    sun_color *= 20;
    sun_color = pow(sun_color, float3(1.3));
    sun_color *= min(smoothstep(0.0, 0.2, clamp(frame_info.sun_direction.y, 0.1, 1.0)) + 0.3, 1.0);
    sun_color /= 40;
    sun_color *= sun_shadow;

    const float Roughness_Amplitude = 0.4;

    float4 base_color = texture(block_atlas, float3(tex_coords, block_face));
    float metallic = 0;
    float roughness = 0.9 - length(base_color.rgb) * Roughness_Amplitude;

    float3 brdf = CalculateBRDF(base_color.rgb, metallic, roughness, N, V, L, sun_color);

    // @Todo: this is a simplification
    float3 sky_irradiance = GetColorNoToneMapping(frame_info.sky, sky_transmittance_LUT, sky_color_LUT, -frame_info.sun_direction, cross(-frame_info.sun_direction, float3(1,0,0)));
    float3 ambient = CalculateAmbientDiffuseBRDF(base_color.rgb, metallic, roughness, N, V, sky_irradiance / Pi);
    ambient *= lerp(0.3, 1.0, occlusion);

    float3 color = ambient + brdf;

    float3 fog_color = float3(1,1,1);
    float fog_distance  = length(position - frame_info.camera.position);
    float fog_value = 1 - exp(-pow(0.001 * fog_distance, 2));
    fog_value = clamp(fog_value, 0, 1);

    color = lerp(color, fog_color, fog_value);

    frag_color = float4(color, base_color.a);
}
