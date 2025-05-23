#version 460

#include "common.glsl"

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec2 position;

void main()
{
    position = Screen_Space_Position[gl_VertexID];
    gl_Position = vec4(position * 2 - vec2(1), 0, 1);
}
