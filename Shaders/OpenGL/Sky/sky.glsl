#include "../common.glsl"

// Azimuth: 0 = positive Z
// Polar: 0 = horizon, Pi/2 = North, -Pi/2 = South

float2 CartesianToSpherical(float3 direction)
{
    float polar = Asin(direction.y);
    float azimuth = Acos(direction.z / length(direction.xz));
    // Cannot use sign because it can return 0, and I'm not sure it handles -0.0
    azimuth *= direction.x < -0.0 ? -1.0 : 1.0;

    return float2(azimuth, polar);
}

float2 CartesianToSphericalUV(float3 direction)
{
    float polar = Asin(direction.y);
    float azimuth = Acos(direction.z / length(direction.xz));
    // Cannot use sign because it can return 0, and I'm not sure it handles -0.0
    azimuth *= direction.x < -0.0 ? -1.0 : 1.0;

    return float2(azimuth / Pi * 0.5 + 0.5, polar / Pi + 0.5);
}

// How to make a spherical to cartesian function for a specific
// coordinate system:
// * if polar at 0 == horizon then use sin on polar axis, otherwise use
// cos (because sin(0) == 0, sin(+/-Pi * 0.5) == +/-1)
// * use cos on the axis where the azimuth being 0 == that axis, use
// sin on the other axis (because cos(0) == 1)
// * use - where axes should be negated (obviously)
//
// Example:
// * polar == 0 -> horizon, azimuth == 0 -> +Z, +Y is up:
//   (sin(azimuth) * cos(polar), sin(polar), cos(azimuth) * cos(polar))

float3 SphericalToCartesian(float azimuth, float polar)
{
    float cosa = cos(azimuth);
    float sina = sin(azimuth);
    float cosp = cos(polar);
    float sinp = sin(polar);

    return float3(sina * cosp, sinp, cosa * cosp);
}

// The first texture in a color LUT is reserved for when the sun is
// completely set, so we return -Pi * 0.5 for angle < LUT_start_angle
// Then we remap [LUT_start_angle; LUT_end_angle] to [0; 1], square it
// to increase the resolution around LUT_start_angle (to capture details
// such as the shadow of the earth at the opposite side of the sunset).
// We then remap the result to [texel_size; 1], where texel_size is 1
// LUT layer.

// float ColorLUTAngleToDepth(in SkyAtmosphere sky, float angle)
// {
//     float texel_size = 1 / float(sky.num_LUT_angles);
//     if (angle < sky.LUT_start_angle)
//         return 0;

//     float v = InverseLerp(sky.LUT_start_angle, sky.LUT_end_angle, angle);

//     v = sqrt(v);

//     return lerp(texel_size, 1.0, v);
// }

// float ColorLUTDepthToAngle(in SkyAtmosphere sky, float depth)
// {
//     float v = depth;

//     float texel_size = 1 / float(sky.num_LUT_angles - 1);
//     if (v < texel_size)
//         return -Pi * 0.5;

//     v = InverseLerp(texel_size, 1.0, v);
//     v = v * v;

//     return lerp(sky.LUT_start_angle, sky.LUT_end_angle, v);
// }

float2 UVToSpherical(float2 uv)
{
    float u = uv.x;
    float v = uv.y;

    // Non-linear mapping of altitude. See Section 5.3 of the paper
    v = lerp(-1, 1, v);
    v = sign(v) * v * v;
    v = InverseLerp(-1, 1, v);

    float azimuth = lerp(-Pi, Pi, u);
    float polar = lerp(-Pi * 0.5, Pi * 0.5, v);

    return float2(azimuth, polar);
}

float2 SphericalToUV(float azimuth, float polar)
{
    float u = InverseLerp(-Pi, Pi, azimuth);
    float v = InverseLerp(-Pi * 0.5, Pi * 0.5, polar);

    // Non-linear mapping of altitude. See Section 5.3 of the paper
    v = lerp(-1, 1, v);
    v = sign(v) * sqrt(abs(v));
    v = InverseLerp(-1, 1, v);

    return float2(u, v);
}

void GetScatteringValues(
    in SkyAtmosphere sky,
    float3 position,
    out float3 rayleigh_scattering,
    out float mie_scattering,
    out float3 extinction
)
{
    float altitude_KM = (length(position) - sky.ground_radius) * 1000;
    float rayleigh_density = exp(-altitude_KM / 8.0);
    float mie_density = exp(-altitude_KM / 1.2);

    rayleigh_scattering = sky.rayleigh_scattering_coeff * rayleigh_density;
    float rayleigh_absorption = sky.rayleigh_absorption_coeff * rayleigh_density;

    mie_scattering = sky.mie_scattering_coeff * mie_density;
    float mie_absorption = sky.mie_absorption_coeff * mie_density;

    float3 ozone_absorption = sky.ozone_absorption_coeff * max(0.0, 1 - abs(altitude_KM - 25) / 15);

    extinction = rayleigh_scattering + rayleigh_absorption + mie_scattering + mie_absorption + ozone_absorption;
}

float GetMiePhase(in SkyAtmosphere sky, float cos_theta)
{
    const float Scale = 3 / (8 * Pi);

    float G = sky.mie_asymmetry_value;
    float num = (1 - G * G) * (1 + cos_theta * cos_theta);
    float denom = (2 + G * G) * pow((1 + G * G - 2 * G * cos_theta), 1.5);

    return Scale * num / denom;
}

float GetRayleighPhase(float cos_theta)
{
    const float K = 3 / (16 * Pi);

    return K * (1 + cos_theta * cos_theta);
}

float3 SampleLUT(
    in SkyAtmosphere sky,
    in sampler2D LUT,
    float3 position,
    float3 sun_direction
)
{
    float height = length(position);
    float3 up = position / height;
    float sun_cos_zenith_angle = dot(sun_direction, up);
    float2 uv = float2(
        clamp(0.5 + 0.5 * sun_cos_zenith_angle, 0.0, 1.0),
        clamp((height - sky.ground_radius) / (sky.atmosphere_radius - sky.ground_radius), 0.0, 1.0)
    );

    return texture(LUT, uv).rgb;
}

// float3 SampleColorLUT(
//     in SkyAtmosphere sky,
//     in sampler3D LUT,
//     float3 sun_direction,
//     float3 ray_direction
// )
// {
//     float2 sun_angles = CartesianToSpherical(sun_direction);
//     float2 angles = CartesianToSpherical(ray_direction);
//     float azimuth_angle = angles.x - sun_angles.x;
//     float polar_angle = angles.y;

//     float height = sky.ground_level;
//     float horizon_angle = Acos(sqrt(height * height - sky.ground_radius * sky.ground_radius) / height) - 0.5 * Pi;
//     polar_angle -= horizon_angle;

//     float LUT_depth = ColorLUTAngleToDepth(sun_angles.y);

//     float3 uvw = float3(SphericalToUV(azimuth_angle, polar_angle), LUT_depth);

//     return texture(LUT, uvw).rgb;
// }

// float3 SampleEnvironmentMap(
//     in SkyAtmosphere sky,
//     in sampler2DArray environment_maps,
//     float3 sun_direction,
//     float3 ray_direction,
//     float roughness
// )
// {
//     float2 sun_angles = CartesianToSpherical(sun_direction);
//     float2 angles = CartesianToSpherical(ray_direction);
//     float azimuth_angle = angles.x - sun_angles.x;
//     float polar_angle = angles.y;

//     float height = sky.ground_level;
//     float horizon_angle = Acos(sqrt(height * height - sky.ground_radius * sky.ground_radius) / height) - 0.5 * Pi;
//     polar_angle -= horizon_angle;

//     float LUT_depth = ColorLUTAngleToDepth(sun_angles.y) * sky.num_LUT_angles;

//     // The environment map is a texture 2D array, so we need
//     // to do the linear filtering ourselves
//     int LUT_depth1 = int(LUT_depth);
//     int LUT_depth2 = LUT_depth1 + 1;
//     float t = LUT_depth - float(LUT_depth1);

//     float3 uvw1 = float3(SphericalToUV(azimuth_angle, polar_angle), float(LUT_depth1));
//     float3 uvw2 = float3(SphericalToUV(azimuth_angle, polar_angle), float(LUT_depth2));

//     float3 sample1 = textureLod(environment_maps, uvw1, roughness * sky.num_environment_map_levels).rgb;
//     float3 sample2 = textureLod(environment_map, uvw2, roughness * sky.num_environment_map_levels).rgb;

//     return lerp(sample1, sample2, t);
// }

float3 SunWithBloom(float3 ray_direction, float3 sun_direction)
{
    const float Sun_Solid_Angle = 0.53 * Pi / 180;
    const float Min_Sun_Cos_Theta = cos(Sun_Solid_Angle);

    float cos_theta = dot(ray_direction, sun_direction);
    if (cos_theta >= Min_Sun_Cos_Theta)
        return float3(1);

    float offset = Min_Sun_Cos_Theta - cos_theta;
    float gaussian_bloom = exp(-offset * 50000) * 0.5;
    float inv_bloom = 1 / (0.02 + offset * 300) * 0.01;

    return float3(gaussian_bloom + inv_bloom);
}

// float3 GetColorNoSunNoToneMapping(
//     float3 sun_direction,
//     float3 ray_direction
// )
// {
//     return SampleColorLUT(color_LUTs, sun_direction, ray_direction);
// }

// float3 GetColorNoToneMapping(
//     float3 sun_direction,
//     float3 ray_direction
// )
// {
//     float3 lum = GetColorNoSunNoToneMapping(sun_direction, ray_direction);

//     // Bloom should be added at the end, but this is subtle and works well.
//     float3 sun_lum = SunWithBloom(ray_direction, sun_direction);
//     // Use smoothstep to limit the effect, so it drops off to actual zero.
//     sun_lum = smoothstep(0.002, 1.0, sun_lum);
//     if (length(sun_lum) > 0)
//     {
//         float height = sky.ground_level;
//         float3 view_position = float3(0, height, 0);

//         if (RayIntersectsSphere(view_position, ray_direction, sky.ground_radius) >= 0)
//         {
//             sun_lum = float3(0);
//         }
//         else
//         {
//             // If the sun value is applied to this pixel, we need to calculate the transmittance to obscure it.
//             sun_lum *= SampleLUT(sky, transmittance_LUT, view_position, sun_direction);
//         }
//     }

//     lum += sun_lum;

//     return lum;
// }

// float3 GetColor(
//     float3 sun_direction,
//     float3 ray_direction
// )
// {
//     float3 lum = GetColorNoToneMapping(sun_direction, ray_direction);

//     lum *= 20;
//     lum = pow(lum, float3(1.3));
//     lum /= (smoothstep(0.0, 0.2, clamp(sun_direction.y, 0.0, 1.0)) * 2 + 0.15);

//     return lum;
// }

// float3 GetColorNoSun(
//     float3 sun_direction,
//     float3 ray_direction
// )
// {
//     float3 lum = SampleColorLUT(color_LUTs, sun_direction, ray_direction);

//     lum *= 20;
//     lum = pow(lum, float3(1.3));
//     lum /= (smoothstep(0.0, 0.2, clamp(sun_direction.y, 0.0, 1.0)) * 2 + 0.15);

//     return lum;
// }
