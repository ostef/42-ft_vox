#define Shadow_Map_Num_Filtering_Samples_Sqrt 8
#define Shadow_Map_Num_Filtering_Samples (Shadow_Map_Num_Filtering_Samples_Sqrt * Shadow_Map_Num_Filtering_Samples_Sqrt)

int GetShadowCascadeIndex(in ShadowMap params, float3 position, float3 normal, out float3 coords)
{
    float3 normal_offset = normal / float(params.resolution) * params.normal_bias;

    int cascade_index = 0;
    for (; cascade_index < Shadow_Map_Num_Cascades; cascade_index += 1)
    {
        float4 light_space_pos = params.cascade_matrices[cascade_index] * float4(position + normal_offset, 1);
        coords = light_space_pos.xyz / light_space_pos.w;
        coords.xy = coords.xy * 0.5 + float2(0.5);

        if (coords.x > 0 && coords.x < 1
         && coords.y > 0 && coords.y < 1
         && coords.z > 0 && coords.z < 1)
        {
            return cascade_index;
        }
    }

    return -1;
}

float3 GetShadowCascadeColor(in ShadowMap params, float3 position, float3 normal)
{
    float3 coords;
    int cascade_index = GetShadowCascadeIndex(params, position, normal, coords);
    if (cascade_index < 0)
        return float3(1);

    return RandomColor((cascade_index + 1) * 1234.5678);
}

float SampleShadowMap(
    in ShadowMap params, in sampler2DArray noise, in sampler2DArrayShadow shadow_map,
    float3 light_direction, float3 world_position, float3 world_normal, float2 screen_position
)
{
    float2 shadow_map_texel_size = 1 / float2(params.resolution, params.resolution);
    float2 noise_texel_size = 1 / float2(params.noise_resolution, params.noise_resolution);

    float3 coords;
    int cascade_index = GetShadowCascadeIndex(params, world_position, world_normal, coords);
    if (cascade_index < 0)
        return 0;

    float shadow_map_size = params.cascade_sizes[cascade_index];

    float NdotL = dot(world_normal, -light_direction);
    float bias_factor = clamp(1.0 - NdotL, 0.0, 1.0);
    float depth_bias = mix(params.depth_bias_min_max.x, params.depth_bias_min_max.y, bias_factor);
    depth_bias *= shadow_map_texel_size.x;
    depth_bias /= shadow_map_size / 20;

    float3 forward = float3(0,0,1);
    float3 right = cross(world_normal, forward);
    forward = cross(right, world_normal);

    float filter_radius = shadow_map_texel_size.x * params.filter_radius / shadow_map_size * 20;

    float shadow_value = 0;
    for (int y = 0; y < Shadow_Map_Num_Filtering_Samples_Sqrt; y += 1)
    {
        for (int x = 0; x < Shadow_Map_Num_Filtering_Samples_Sqrt; x += 1)
        {
            float3 noise_coords = float3(screen_position * noise_texel_size, y * Shadow_Map_Num_Filtering_Samples_Sqrt + x);

            float2 offset = texture(noise, noise_coords).xy;
            offset *= filter_radius;

            float3 uvw = float3(coords.xy + offset, cascade_index);
            shadow_value += texture(shadow_map, float4(uvw, coords.z - depth_bias));
        }
    }

    return shadow_value / float(Shadow_Map_Num_Filtering_Samples);
}
