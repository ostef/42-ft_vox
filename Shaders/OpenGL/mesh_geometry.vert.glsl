#version 460

#include "common.glsl"

layout(location = 0) in vec3 v_position;
layout(location = 1) in Block v_block;
layout(location = 2) in BlockFace v_block_face;
layout(location = 3) in BlockCorner v_block_corner;

layout(std140) uniform frame_info_buffer
{
    FrameInfo frame_info;
};

out gl_PerVertex
{
    vec4 gl_Position;
};

flat out Block block;
flat out BlockFace block_face;
out vec2 screen_position;
out vec3 position;
out vec3 normal;
out vec2 tex_coords;

void main()
{
    int num_atlas_blocks = int(frame_info.texture_atlas_size.x / frame_info.texture_block_size.x);
    vec2 tex_coords_start = vec2((v_block - 1) % num_atlas_blocks, (v_block - 1) / num_atlas_blocks);

    block = v_block;
    block_face = v_block_face;
    position = v_position;
    normal = Block_Normals[v_block_face];
    tex_coords = (Block_Tex_Coords[v_block_corner] + tex_coords_start) / float(num_atlas_blocks);

    gl_Position = frame_info.camera.projection * frame_info.camera.view * vec4(v_position,1);
    screen_position = (gl_Position.xy + vec2(1)) * 0.5;
}
