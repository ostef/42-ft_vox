#ifndef PBR_GLSL
#define PBR_GLSL

// https://graphicscompendium.com/gamedev/15-pbr
// https://graphicscompendium.com/references/cook-torrance
// https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf

// https://pharr.org/matt/blog/2022/05/06/trowbridge-reitz
float DistributionThrowbridgeReitz(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);

    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = Pi * denom * denom;

    return a2 / denom;
}

// The geometry function returns a value between 0 and 1 that approximates how much light is blocked by
// microfacets based on the angle between the light ray and the macrosurface
// This is the direct lighting version of the geometry function
// The k value is computed differently for IBL (https://learnopengl.com/PBR/Theory)
// Direct: k = (a + 1)^2 / 8
// IBL:    k = a^2 / 2
float GeometrySchlick(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySchlickForIncomingAndOutgoing(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlick(NdotV, roughness);
    float ggx1 = GeometrySchlick(NdotL, roughness);

    return ggx1 * ggx2;
}

// The fresnel equation describes the ratio of light that is reflected over the light that is refracted
float3 FresnelSchlick(float HdotV, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

float3 FresnelSchlickWithRoughness(float HdotV, float3 F0, float roughness)
{
    return F0 + (max(float3(1 - roughness), F0) - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

float3 CalculateBRDF(
    float3 albedo, float metallic, float roughness,
    float3 N, float3 V, float3 L,
    float3 radiance
)
{
    float3 F0 = float3(0.04);
    F0 = mix(F0, albedo, metallic);

    float3 H = normalize(V + L);

    float NDF = DistributionThrowbridgeReitz(N, H, roughness);
    float G   = GeometrySchlickForIncomingAndOutgoing(N, V, L, roughness);
    float3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    float3 specular = numerator / denominator;
    float3 diffuse = kD * albedo / Pi;

    float3 light_Lo = (diffuse + specular) * radiance * NdotL;

    return light_Lo;
}

float3 CalculateAmbientDiffuseBRDF(
    float3 albedo, float metallic, float roughness,
    float3 N, float3 V,
    float3 irradiance
)
{
    float NdotV = max(dot(N, V), 0.0);
    // Irradiance comes from all directions, hence L == N:
    // L = N
    // H = V + L = V + N
    float HdotV = max(dot(V + N, V), 0.0);
    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 F = FresnelSchlickWithRoughness(HdotV, F0, roughness);

    float3 diffuse = irradiance * albedo;

    float3 kS = F;
    float3 kD = float3(1) - kS;
    kD *= 1.0 - metallic;

    return kD * diffuse;
}

#endif
