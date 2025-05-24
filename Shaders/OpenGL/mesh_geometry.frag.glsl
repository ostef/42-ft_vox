#version 460

#include "common.glsl"

in vec3 normal;

out vec4 frag_color;

void main()
{
    const vec3 light_dir = normalize(vec3(1,-1,0.3));

    float light_intensity = max(dot(normal, light_dir), 0);
    vec3 color = vec3(normal);

    frag_color = vec4(color,1);
}
