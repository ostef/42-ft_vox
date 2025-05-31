#version 460

#include "sky.glsl"

layout(std140) uniform sky_buffer
{
    SkyAtmosphere sky;
};

uniform sampler2D transmittance_LUT;

in float2 position;

out float4 frag_color;

void GetMultiScatterValues(float3 position, float3 sun_direction, out float3 lum_total, out float3 fms)
{
    lum_total = float3(0);
    fms = float3(0);

    const int Sqrt_Samples = 8;

    float inv_samples = 1 / float(Sqrt_Samples * Sqrt_Samples);
    for (int i = 0; i < Sqrt_Samples; i++)
    {
        for (int j = 0; j < Sqrt_Samples; j++)
        {
            // This integral is symmetric about theta = 0(or theta = PI),
            // so we only need to integrate from zero to PI, not zero to 2*PI
            float theta = Pi * (float(i) + 0.5) / float(Sqrt_Samples);
            float phi = Acos(1 - 2 * (float(j) + 0.5) / float(Sqrt_Samples));
            float3 ray_direction = SphericalToCartesian(theta, phi);

            float atmosphere_dist = RayIntersectsSphere(position, ray_direction, sky.atmosphere_radius);
            float ground_dist = RayIntersectsSphere(position, ray_direction, sky.ground_radius);
            float t_max = atmosphere_dist;
            if (ground_dist > 0)
                t_max = ground_dist;

            float cos_theta = dot(ray_direction, sun_direction);

            float mie_phase_value = GetMiePhase(sky, cos_theta);
            float rayleigh_phase_value = GetRayleighPhase(-cos_theta);

            float3 lum = float3(0);
            float3 lum_factor = float3(0);
            float3 transmittance = float3(1);
            float t = 0;
            for (float s = 0; s < float(sky.num_multi_scatter_steps); s += 1)
            {
                float new_t = ((s + 0.3) / float(sky.num_multi_scatter_steps)) * t_max;
                float delta_t = new_t - t;
                t = new_t;

                float3 new_pos = position + t * ray_direction;

                float3 rayleigh_scattering, extinction;
                float mie_scattering;
                GetScatteringValues(sky, new_pos, rayleigh_scattering, mie_scattering, extinction);

                float3 sample_transmittance = exp(-delta_t * extinction);

                // Integrate within each segment
                float3 scattering_no_phase = rayleigh_scattering + mie_scattering;
                float3 scattering_factor = (scattering_no_phase - scattering_no_phase * sample_transmittance) / extinction;
                lum_factor += transmittance * scattering_factor;

                // This is slightly different from the paper, but I think
                // the paper has a mistake?
                // In equation(6), I think S(x,w_s) should be S(x-tv,w_s).
                float3 sun_transmittance = SampleLUT(sky, transmittance_LUT, new_pos, sun_direction);

                float3 rayleigh_in_scattering = rayleigh_scattering * rayleigh_phase_value;
                float mie_in_scattering = mie_scattering * mie_phase_value;
                float3 in_scattering = (rayleigh_in_scattering + mie_in_scattering) * sun_transmittance;

                // Integrated scattering within path segment
                float3 scattering_integral = (in_scattering - in_scattering * sample_transmittance) / extinction;

                lum += scattering_integral * transmittance;
                transmittance *= sample_transmittance;
            }

            if (ground_dist > 0)
            {
                float3 hit_pos = position + ground_dist * ray_direction;
                if (dot(position, sun_direction) > 0)
                {
                    hit_pos = normalize(hit_pos) * sky.ground_radius;
                    lum += transmittance * sky.ground_albedo * SampleLUT(sky, transmittance_LUT, hit_pos, sun_direction);
                }
            }

            fms += lum_factor * inv_samples;
            lum_total += lum * inv_samples;
        }
    }
}

void main()
{
    float u = position.x;
    float v = position.y;

    float sun_cos_theta = 2 * u - 1;
    float sun_theta = Acos(sun_cos_theta);
    float height = lerp(sky.ground_radius, sky.atmosphere_radius, v);

    float3 sample_position = float3(0, height, 0);
    float3 sun_direction = normalize(float3(0, sun_cos_theta, -sin(sun_theta)));

    float3 lum, fms;
    GetMultiScatterValues(sample_position, sun_direction, lum, fms);

    // Equation 10 from the paper
    float3 psi = lum / (float3(1) - fms);

    frag_color = float4(psi, 1);
}
