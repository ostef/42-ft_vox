#version 460

#include "common.glsl"

layout(location = 0) in vec3 v_position;
layout(location = 1) in BlockFace v_block_face;

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec3 normal;

void main()
{
    normal = Block_Normals[v_block_face];

    gl_Position = frame_info.camera.projection * frame_info.camera.view * vec4(v_position,1);
}
