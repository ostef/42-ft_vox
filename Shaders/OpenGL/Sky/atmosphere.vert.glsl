#version 460

#include "../common.glsl"

out gl_PerVertex
{
    float4 gl_Position;
};

out float2 position;

void main()
{
    position = Screen_Space_Position[gl_VertexID];
    gl_Position = float4(position * 2 - float2(1), 1, 1);
}
