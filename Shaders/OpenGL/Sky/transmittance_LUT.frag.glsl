#version 460

#include "sky.glsl"

layout(std140) uniform sky_buffer
{
    SkyAtmosphere sky;
};

in float2 position;

out float4 frag_color;

float3 GetSunTransmittance(float3 position, float3 sun_direction)
{
    if (RayIntersectsSphere(position, sun_direction, sky.ground_radius) > 0)
        return float3(0);

    float atmosphere_dist = RayIntersectsSphere(position, sun_direction, sky.atmosphere_radius);
    float t = 0;

    float3 transmittance = float3(1);
    for (float i = 0; i < float(sky.num_transmittance_steps); i += 1)
    {
        float new_t = ((i + 0.3) / float(sky.num_transmittance_steps)) * atmosphere_dist;
        float delta_t = new_t - t;
        t = new_t;

        float3 new_position = position + t * sun_direction;

        float3 rayleigh_scattering, extinction;
        float mie_scattering;
        GetScatteringValues(sky, new_position, rayleigh_scattering, mie_scattering, extinction);

        transmittance *= exp(-delta_t * extinction);
    }

    return transmittance;
}

void main()
{
    float u = position.x;
    float v = position.y;

    float sun_cos_angle = 2 * u - 1;
    float sun_angle = Acos(sun_cos_angle);
    float height = lerp(sky.ground_radius, sky.atmosphere_radius, v);

    float3 sample_position = float3(0, height, 0);
    float3 sun_direction = normalize(float3(0, sun_cos_angle, -sin(sun_angle)));

    frag_color = float4(GetSunTransmittance(sample_position, sun_direction), 1);
}
