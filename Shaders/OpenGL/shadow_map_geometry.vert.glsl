#version 460

#if defined(GL_AMD_vertex_shader_layer)
    #extension GL_AMD_vertex_shader_layer : require
#endif
#if defined(GL_NV_viewport_array2)
    #extension GL_NV_viewport_array2 : require
#endif
#if !defined(GL_AMD_vertex_shader_layer) && !defined(GL_NV_viewport_array2)
    #error No extension available for usage of gl_Layer inside vertex shader.
#endif

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
    int gl_Layer;
};

void main()
{
    int cascade_index = gl_InstanceID % 4;
    gl_Position = frame_info.shadow_map.cascade_matrices[cascade_index] * vec4(v_position, 1);
    gl_Layer = cascade_index;
}
