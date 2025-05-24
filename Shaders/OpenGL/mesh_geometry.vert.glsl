#version 460

#include "common.glsl"

layout(location = 0) in vec3 v_position;
layout(location = 1) in BlockFace v_block_face;

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

layout(std430) readonly buffer chunk_info_buffer
{
    ChunkInfo chunk_infos[];
};

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec3 normal;

void main()
{
    ChunkInfo chunk_info = chunk_infos[gl_BaseInstance + gl_InstanceID];

    normal = (chunk_info.transform * vec4(Block_Normals[v_block_face], 0)).xyz;

    gl_Position = frame_info.camera.projection * frame_info.camera.view * chunk_info.transform * vec4(v_position,1);
}
