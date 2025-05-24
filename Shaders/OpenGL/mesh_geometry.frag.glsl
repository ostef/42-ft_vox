#version 460

#include "common.glsl"

uniform sampler2DArray block_atlas;

flat in Block block;
flat in BlockFace block_face;
in vec2 tex_coords;
in vec3 normal;

out vec4 frag_color;

void main()
{
    const vec3 light_dir = normalize(vec3(1,1,0.3));

    float light_intensity = max(dot(normal, light_dir), 0);
    vec3 color = texture(block_atlas, vec3(tex_coords, block_face)).rgb;
    // vec3 color = vec3(tex_coords, 0);

    frag_color = vec4(color,1);
}
