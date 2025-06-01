#version 460

#include "sky.glsl"

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

layout(binding=0) uniform sampler2D transmittance_LUT;
layout(binding=1) uniform sampler2D multi_scatter_LUT;

in float2 position;

out float4 frag_color;

float3 RaymarchScattering(
    float3 position,
    float3 ray_direction,
    float3 sun_direction,
    float t_max,
    float num_steps
)
{
    float cos_theta = dot(ray_direction, sun_direction);

    float mie_phase_value = GetMiePhase(frame_info.sky, cos_theta);
    float rayleigh_phase_value = GetRayleighPhase(-cos_theta);

    float3 lum = float3(0);
    float3 transmittance = float3(1);
    float t = 0;
    for (float i = 0; i < num_steps; i += 1)
    {
        float new_t = ((i + 0.3) / num_steps) * t_max;
        float delta_t = new_t - t;
        t = new_t;

        float3 new_pos = position + t * ray_direction;

        float3 rayleigh_scattering, extinction;
        float mie_scattering;
        GetScatteringValues(frame_info.sky, new_pos, rayleigh_scattering, mie_scattering, extinction);

        float3 sample_transmittance = exp(-delta_t * extinction);

        float3 sun_transmittance = SampleLUT(frame_info.sky, transmittance_LUT, new_pos, sun_direction);
        float3 psi_multi_scatter = SampleLUT(frame_info.sky, multi_scatter_LUT, new_pos, sun_direction);

        float3 rayleigh_in_scattering = rayleigh_scattering * (rayleigh_phase_value * sun_transmittance + psi_multi_scatter);
        float3 mie_in_scattering = mie_scattering * (mie_phase_value * sun_transmittance + psi_multi_scatter);
        float3 in_scattering = rayleigh_in_scattering + mie_in_scattering;

        // Integrated scattering within path segment
        float3 scattering_integral = (in_scattering - in_scattering * sample_transmittance) / extinction;

        lum += scattering_integral * transmittance;
        transmittance *= sample_transmittance;
    }

    return lum;
}

void main()
{
    float2 spherical = UVToSpherical(position);
    float azimuth_angle = spherical.x;
    float polar_angle = spherical.y;

    float height = frame_info.sky.ground_level;
    float3 view_position = float3(0, height, 0);
    float3 up = view_position / height;
    float horizon_angle = Acos(sqrt(height * height - frame_info.sky.ground_radius * frame_info.sky.ground_radius) / height) - 0.5 * Pi;
    polar_angle -= horizon_angle;

    float3 ray_direction = SphericalToCartesian(azimuth_angle, polar_angle);
    float3 sun_direction = SphericalToCartesian(0, frame_info.sun_polar);

    float atmosphere_dist = RayIntersectsSphere(view_position, ray_direction, frame_info.sky.atmosphere_radius);
    float ground_dist = RayIntersectsSphere(view_position, ray_direction, frame_info.sky.ground_radius);
    float t_max = (ground_dist < 0) ? atmosphere_dist : ground_dist;
    float3 lum = RaymarchScattering(
        view_position,
        ray_direction,
        sun_direction,
        t_max,
        float(frame_info.sky.num_multi_scatter_steps)
    );

    frag_color = float4(lum,1);
}
