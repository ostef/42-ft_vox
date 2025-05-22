#version 450 core

layout(std140) uniform frame_info_buffer
{
    vec2 viewport_size;
    mat4 view;
    mat4 inv_view;
    mat4 projection;
    mat4 inv_projection;
} frame_info;

layout(std430) buffer skinning_matrices_buffer
{
    mat4 matrices[];
};

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = frame_info.projection * frame_info.view * matrices[0] * vec4(1,1,1,1);
}
