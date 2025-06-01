#ifndef FXAA_GLSL
#define FXAA_GLSL

// I don't really understand this yet, I just implemented it quickly from this reference
// https://blog.simonrodriguez.fr/articles/2016/07/implementing_fxaa.html
// This algorithm detects edges by looking at the luminance value of the neighboring
// pixels, and blurring along the direction of the edge
// @Todo: make this better

float RGBA2Luma(float4 rgba)
{
    return LinearRGBToLuminance(rgba.rgb);
}

#define FXAA_Edge_Threshold_Min 0.0312
#define FXAA_Edge_Threshold_Max 0.125
#define FXAA_Iterations 12
#define FXAA_Subpixel_Quality 0.75

float4 FXAA(sampler2D tex, float2 tex_coords, float2 texel_size)
{
    const float FXAA_Quality[] = float[](1, 1, 1, 1, 1, 1.5, 2, 2, 2, 2, 4, 8);

    float4 color_center = texture(tex, tex_coords);

    float luma_center = RGBA2Luma(color_center);

    float luma_up = RGBA2Luma(texture(tex, tex_coords + float2(0, texel_size.y)));
    float luma_down = RGBA2Luma(texture(tex, tex_coords + float2(0, -texel_size.y)));
    float luma_left = RGBA2Luma(texture(tex, tex_coords + float2(-texel_size.x, 0)));
    float luma_right = RGBA2Luma(texture(tex, tex_coords + float2(texel_size.x, 0)));

    float luma_min = min(luma_center, min(luma_up, min(luma_down, min(luma_left, luma_right))));
    float luma_max = max(luma_center, max(luma_up, max(luma_down, max(luma_left, luma_right))));

    float luma_range = luma_max - luma_min;

    if (luma_range < max(FXAA_Edge_Threshold_Min, luma_max * FXAA_Edge_Threshold_Max))
        return color_center;

    float luma_up_right = RGBA2Luma(texture(tex, tex_coords + float2(texel_size.x, texel_size.y)));
    float luma_up_left = RGBA2Luma(texture(tex, tex_coords + float2(-texel_size.x, texel_size.y)));
    float luma_down_right = RGBA2Luma(texture(tex, tex_coords + float2(texel_size.x, -texel_size.y)));
    float luma_down_left = RGBA2Luma(texture(tex, tex_coords + float2(-texel_size.x, -texel_size.y)));

    float luma_down_up = luma_down + luma_up;
    float luma_left_right = luma_left + luma_right;

    float luma_up_corners = luma_up_left + luma_up_right;
    float luma_down_corners = luma_down_left + luma_down_right;
    float luma_right_corners = luma_down_right + luma_up_right;
    float luma_left_corners = luma_down_left + luma_up_left;

    float edge_horizontal = abs(-2 * luma_left + luma_left_corners) + abs(-2 * luma_center + luma_down_up) * 2 + abs(-2 * luma_right + luma_right_corners);
    float edge_vertical = abs(-2 * luma_up + luma_up_corners) + abs(-2 * luma_center + luma_left_right) * 2 + abs(-2 * luma_down + luma_down_corners);

    bool is_horizontal = edge_horizontal >= edge_vertical;

    float luma1 = is_horizontal ? luma_down : luma_left;
    float luma2 = is_horizontal ? luma_up : luma_right;

    float gradient1 = luma1 - luma_center;
    float gradient2 = luma2 - luma_center;

    bool is_1_steepest = abs(gradient1) >= abs(gradient2);

    float gradient_scaled = 0.25 * max(abs(gradient1), abs(gradient2));

    float step_length = is_horizontal ? texel_size.y : texel_size.x;

    float luma_local_average = 0;

    if (is_1_steepest)
    {
        step_length = -step_length;
        luma_local_average = 0.5 * (luma1 + luma_center);
    }
    else
    {
        luma_local_average = 0.5 * (luma2 + luma_center);
    }

    float2 current_uv = tex_coords;
    if (is_horizontal)
        current_uv.y += step_length * 0.5;
    else
        current_uv.x += step_length * 0.5;

    float2 offset = is_horizontal ? float2(texel_size.x, 0) : float2(0, texel_size.y);
    float2 uv1 = current_uv - offset;
    float2 uv2 = current_uv + offset;

    float luma_end1 = RGBA2Luma(texture(tex, uv1));
    float luma_end2 = RGBA2Luma(texture(tex, uv2));
    luma_end1 -= luma_local_average;
    luma_end2 -= luma_local_average;

    bool reached1 = abs(luma_end1) >= gradient_scaled;
    bool reached2 = abs(luma_end2) >= gradient_scaled;
    bool reached_both = reached1 && reached2;

    if (!reached1)
        uv1 -= offset;
    if (!reached2)
        uv2 += offset;

    if (!reached_both)
    {
        for (int i = 2; i < FXAA_Iterations; i += 1)
        {
            if (!reached1)
            {
                luma_end1 = RGBA2Luma(texture(tex, uv1));
                luma_end1 = luma_end1 - luma_local_average;
            }

            if (!reached2)
            {
                luma_end2 = RGBA2Luma(texture(tex, uv2));
                luma_end2 = luma_end2 - luma_local_average;
            }

            reached1 = abs(luma_end1) >= gradient_scaled;
            reached2 = abs(luma_end2) >= gradient_scaled;
            reached_both = reached1 && reached2;

            if (!reached1)
                uv1 -= offset * FXAA_Quality[i];
            if (!reached2)
                uv2 += offset * FXAA_Quality[i];

            if (reached_both)
                break;
        }
    }

    float distance1 = is_horizontal ? (tex_coords.x - uv1.x) : (tex_coords.y - uv1.y);
    float distance2 = is_horizontal ? (uv2.x - tex_coords.x) : (uv2.y - tex_coords.y);

    bool is_direction1 = distance1 < distance2;
    float distance_final = min(distance1, distance2);

    float edge_thickness = (distance1 + distance2);

    float pixel_offset = -distance_final / edge_thickness + 0.5;

    bool is_luma_center_smaller = luma_center < luma_local_average;

    bool correct_variation = ((is_direction1 ? luma_end1 : luma_end2) < 0) != is_luma_center_smaller;

    float final_offset = correct_variation ? pixel_offset : 0;

    float luma_average = (1.0 / 12.0) * (2 * (luma_down_up + luma_left_right) + luma_left_corners + luma_right_corners);

    float sub_pixel_offset1 = clamp(abs(luma_average - luma_center) / luma_range, 0, 1);
    float sub_pixel_offset2 = (-2 * sub_pixel_offset1 + 3) * sub_pixel_offset1 * sub_pixel_offset1;

    float sub_pixel_offset_final = sub_pixel_offset2 * sub_pixel_offset2 * FXAA_Subpixel_Quality;

    final_offset = max(final_offset, sub_pixel_offset_final);

    float2 final_uv = tex_coords;
    if (is_horizontal)
        final_uv.y += final_offset * step_length;
    else
        final_uv.x += final_offset * step_length;

    return texture(tex, final_uv);
}

#endif
