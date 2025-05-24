#include "Graphics.hpp"

static GLuint GLGetFramebuffer(GfxRenderPassDesc desc);

GfxRenderPassDesc GetDesc(GfxRenderPass *pass)
{
    return pass->desc;
}

GfxRenderPass GfxBeginRenderPass(String name, GfxCommandBuffer *cmd_buffer, GfxRenderPassDesc desc)
{
    GfxBeginDebugGroup(cmd_buffer, name);

    GfxRenderPass pass {};
    pass.name = name;
    pass.desc = desc;
    pass.cmd_buffer = cmd_buffer;

    pass.fbo = GLGetFramebuffer(desc);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass.fbo);

    glDisable(GL_SCISSOR_TEST); // Prevent scissor from affecting clearing

    int color_buffer_index = 0;
    for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
    {
        if (desc.color_attachments[i])
        {
            glColorMaski(color_buffer_index, true, true, true, true);
            if (desc.should_clear_color[i])
                glClearBufferfv(GL_COLOR, color_buffer_index, (float *)&desc.clear_color[i]);

            color_buffer_index += 1;
        }
    }

    if (desc.depth_attachment && desc.should_clear_depth)
    {
        glDepthMask(true);
        glClearBufferfv(GL_DEPTH, 0, &desc.clear_depth);
    }

    if (desc.stencil_attachment && desc.should_clear_stencil)
    {
        glStencilMask(0xffffffff);
        glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glClearBufferiv(GL_STENCIL, 0, (int *)&desc.clear_stencil);
    }

    return pass;
}

void GfxEndRenderPass(GfxRenderPass *pass)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    GfxEndDebugGroup(pass->cmd_buffer);
}

void GfxSetPipelineState(GfxRenderPass *pass, GfxPipelineState *state)
{
    pass->current_pipeline_state = state;

    glBindProgramPipeline(state->pso);

    // Relocate fragment shader bindings so they do not conflict with the vertex shader's
    if (state->desc.fragment_shader)
    {
        GLuint fragment_program = state->desc.fragment_shader->handle;
        foreach (i, state->fragment_stage_binding_relocations)
        {
            auto binding = state->fragment_stage_bindings[i];
            auto reloc = state->fragment_stage_binding_relocations[i];
            switch (binding.type)
            {
            case GfxPipelineBindingType_UniformBuffer:
                if (reloc.block_index_or_uniform_location >= 0) {
                    glUniformBlockBinding(fragment_program, reloc.block_index_or_uniform_location, reloc.relocated_binding_index);
                }
                break;

            case GfxPipelineBindingType_StorageBuffer:
                if (reloc.block_index_or_uniform_location >= 0) {
                    glShaderStorageBlockBinding(fragment_program, reloc.block_index_or_uniform_location, reloc.relocated_binding_index);
                }
                break;

            case GfxPipelineBindingType_Texture:
            case GfxPipelineBindingType_SamplerState:
                foreach (unit_index, reloc.associated_texture_units)
                {
                    auto unit = reloc.associated_texture_units[unit_index];
                    if (reloc.block_index_or_uniform_location >= 0) {
                        glProgramUniform1i(fragment_program, reloc.block_index_or_uniform_location, reloc.relocated_binding_index);
                    }
                }

                break;

            case GfxPipelineBindingType_CombinedSampler:
                if (reloc.block_index_or_uniform_location >= 0) {
                    glProgramUniform1i(fragment_program, reloc.block_index_or_uniform_location, reloc.relocated_binding_index);
                }
                break;
            }
        }
    }

    glDisable(GL_SCISSOR_TEST); // Enabled when calling set_scissor_rect

    // Vertex layout (i.e. vertex array)
    glBindVertexArray(state->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass->current_index_buffer ? pass->current_index_buffer->handle : 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Rasterizer state
    auto rasterizer = state->desc.rasterizer_state;
    glPolygonMode(GL_FRONT_AND_BACK, GLFillMode(rasterizer.fill_mode));

    if (rasterizer.cull_face == GfxPolygonFace_None)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GLFace(rasterizer.cull_face));
    }

    glFrontFace(GLWindingOrder(rasterizer.winding_order));

    // Blend state
    for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
    {
        auto blend = state->desc.blend_states[i];
        if (!blend.enabled)
        {
            glDisablei(GL_BLEND, i);
            continue;
        }

        glEnablei(GL_BLEND, i);
        glBlendFuncSeparatei(
            i,
            GLBlendFactor(blend.src_RGB), GLBlendFactor(blend.dst_RGB),
            GLBlendFactor(blend.src_alpha), GLBlendFactor(blend.dst_alpha)
        );
        glBlendEquationSeparatei(
            i,
            GLBlendEquation(blend.RGB_operation),
            GLBlendEquation(blend.alpha_operation)
        );
    }

    // Depth state
    auto depth = state->desc.depth_state;
    if (depth.enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GLComparisonFunc(depth.compare_func));
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(depth.write_enabled);

    // Stencil state (@Todo)
}

void GfxSetViewport(GfxRenderPass *pass, GfxViewport viewport)
{
    glViewport(viewport.top_left_x, viewport.top_left_y, viewport.width, viewport.height);
    glDepthRange(viewport.min_depth, viewport.max_depth);
}

void GfxSetScissorRect(GfxRenderPass *pass, Recti rect)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(rect.x, rect.y, rect.w, rect.h);
}

void GfxSetVertexBuffer(GfxRenderPass *Pass, int index, GfxBuffer *buffer, s64 offset, s64 size, s64 stride)
{
    Assert(offset >= 0 && size >= 0 && stride >= 0, "Invalid buffer offset or size or stride");
    Assert(index >= 0, "Invalid buffer index");

    glBindVertexBuffer(index, buffer ? buffer->handle : 0, offset, stride);
}

void GfxSetBuffer(GfxRenderPass *pass, GfxPipelineBinding binding, GfxBuffer *buffer, s64 offset, s64 size)
{
    Assert(offset >= 0 && size >= 0, "Invalid buffer offset or size");

    if (binding.index < 0)
        return;

    Assert(binding.type == GfxPipelineBindingType_UniformBuffer || binding.type == GfxPipelineBindingType_StorageBuffer, "GfxSetBuffer: binding is not a buffer");

    GLuint handle = buffer && size > 0 ? buffer->handle : 0;
    if (binding.type == GfxPipelineBindingType_UniformBuffer)
        glBindBufferRange(GL_UNIFORM_BUFFER, binding.index, handle, offset, size);
    else
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.index, handle, offset, size);
}

void GfxSetTexture(GfxRenderPass *pass, GfxPipelineBinding binding, GfxTexture *texture)
{
    if (binding.index < 0 && binding.associated_texture_units.count <= 0)
        return;

    Assert(binding.type == GfxPipelineBindingType_Texture || binding.type == GfxPipelineBindingType_CombinedSampler, "GfxSetTexture: binding is not a texture or a combined sampler");

    GLuint handle = texture ? texture->handle : 0;
    if (binding.associated_texture_units.count > 0)
    {
        foreach (i, binding.associated_texture_units)
            glBindTextureUnit(binding.associated_texture_units[i], handle);
    }
    else
    {
        glBindTextureUnit(binding.index, handle);
    }
}

void GfxSetSamplerState(GfxRenderPass *pass, GfxPipelineBinding binding, GfxSamplerState *sampler)
{
    if (binding.index < 0 && binding.associated_texture_units.count <= 0)
        return;

    Assert(binding.type == GfxPipelineBindingType_SamplerState || binding.type == GfxPipelineBindingType_CombinedSampler, "GfxSetSamplerState: binding is not a sampler state or a combined sampler");

    GLuint handle = sampler ? sampler->handle : 0;
    if (binding.associated_texture_units.count > 0)
    {
        foreach (i, binding.associated_texture_units)
            glBindSampler(binding.associated_texture_units[i], handle);
    }
    else
    {
        glBindSampler(binding.index, handle);
    }
}

void GfxDrawPrimitives(GfxRenderPass *pass, u32 vertex_count, u32 instance_count, u32 base_vertex, u32 base_instance)
{
    GLenum mode = GL_TRIANGLES;
    switch (pass->current_pipeline_state->desc.rasterizer_state.primitive_type)
    {
    case GfxPrimitiveType_Point:    mode = GL_POINTS;    break;
    case GfxPrimitiveType_Line:     mode = GL_LINES;     break;
    case GfxPrimitiveType_Triangle: mode = GL_TRIANGLES; break;
    }

    glDrawArraysInstancedBaseInstance(
        mode,
        base_vertex,
        vertex_count,
        instance_count,
        base_instance
    );
}

void GfxDrawIndexedPrimitives(GfxRenderPass *pass, GfxBuffer *index_buffer, u32 index_count, GfxIndexType index_type, u32 instance_count, u32 base_vertex, u32 base_index, u32 base_instance)
{
    if (index_buffer != pass->current_index_buffer)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer ? index_buffer->handle : 0);
        pass->current_index_buffer = index_buffer;
    }

    GLenum mode = GL_TRIANGLES;
    switch (pass->current_pipeline_state->desc.rasterizer_state.primitive_type)
    {
    case GfxPrimitiveType_Point:    mode = GL_POINTS;    break;
    case GfxPrimitiveType_Line:     mode = GL_LINES;     break;
    case GfxPrimitiveType_Triangle: mode = GL_TRIANGLES; break;
    }

    GLenum type = 0;
    int index_size = 0;
    switch (index_type)
    {
    case GfxIndexType_Uint32:
        type = GL_UNSIGNED_INT;
        index_size = 4;
        break;
    case GfxIndexType_Uint16:
        type = GL_UNSIGNED_SHORT;
        index_size = 2;
        break;
    }

    glDrawElementsInstancedBaseVertexBaseInstance(
        mode,
        index_count,
        type,
        (void *)(ptrdiff_t)(base_index * index_size),
        instance_count,
        base_vertex,
        base_instance
    );
}

GLuint GLGetFramebuffer(GfxRenderPassDesc desc)
{
    OpenGLFramebufferKey key{};
    bool has_swapchain_texture = false;
    for (int i = 0; i < (int)StaticArraySize(desc.color_attachments); i += 1)
    {
        auto texture = desc.color_attachments[i];
        if (texture == GfxGetSwapchainTexture())
        {
            Assert(i == 0, "Swapchain texture can only be used as the only texture for attachment 0");
            has_swapchain_texture = true;
        }

        key.color_textures[i] = texture ? texture->handle : 0;
    }

    if (has_swapchain_texture)
        Assert(desc.depth_attachment == null && desc.stencil_attachment == null, "Swapchain texture can only be used as the only texture for attachment 0");

    key.depth_texture = desc.depth_attachment ? desc.depth_attachment->handle : 0;
    key.stencil_texture = desc.stencil_attachment ? desc.stencil_attachment->handle : 0;

    if (has_swapchain_texture)
        return 0; // Return default framebuffer

    bool exists = false;
    auto ptr = HashMapFindOrAdd(&g_gfx_context.framebuffer_cache, key, &exists);
    if (exists)
        return *ptr;

    glCreateFramebuffers(1, ptr);

    GLenum draw_buffers[Gfx_Max_Color_Attachments] = {};
    int num_draw_buffers = 0;

    for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
    {
        if (!key.color_textures[i])
            continue;

        glNamedFramebufferTexture(
            *ptr,
            GL_COLOR_ATTACHMENT0 + i,
            key.color_textures[i],
            0
        );

        draw_buffers[num_draw_buffers] = GL_COLOR_ATTACHMENT0 + i;
        num_draw_buffers += 1;
    }

    glNamedFramebufferDrawBuffers(*ptr, num_draw_buffers, draw_buffers);

    if (key.depth_texture)
    {
        glNamedFramebufferTexture(
            *ptr,
            GL_DEPTH_ATTACHMENT,
            key.depth_texture,
            0
        );
    }

    if (key.stencil_texture)
    {
        glNamedFramebufferTexture(
            *ptr,
            GL_STENCIL_ATTACHMENT,
            key.stencil_texture,
            0
        );
    }

    auto status = glCheckNamedFramebufferStatus(*ptr, GL_DRAW_FRAMEBUFFER);
    Assert(status == GL_FRAMEBUFFER_COMPLETE, "Failed to create framebuffer: %x", status);

    return *ptr;
}

void InvalidateFramebuffersUsingTexture(GLuint handle)
{
    foreach(i, g_gfx_context.framebuffer_cache.entries)
    {
        auto entry = &g_gfx_context.framebuffer_cache.entries[i];
        if (entry->hash >= Hash_Map_First_Occupied)
        {
            bool should_remove = false;
            for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
            {
                if (entry->key.color_textures[i] == handle)
                {
                    should_remove = true;
                    break;
                }
            }

            if (should_remove || entry->key.depth_texture == handle || entry->key.stencil_texture == handle)
            {
                entry->hash = Hash_Map_Removed;
                g_gfx_context.framebuffer_cache.count -= 1;
            }
        }
    }
}

GLenum GLFillMode(GfxFillMode mode)
{
    switch (mode)
    {
    case GfxFillMode_Fill:  return GL_FILL;
    case GfxFillMode_Lines: return GL_LINE;
    }
    Panic("Invalid value for mode");
    return 0;
}

GLenum GLFace(GfxPolygonFace face)
{
    switch (face)
    {
    case GfxPolygonFace_None:  return 0;
    case GfxPolygonFace_Front: return GL_FRONT;
    case GfxPolygonFace_Back:  return GL_BACK;
    }
    Panic("Invalid value for face");
    return 0;
}

GLenum GLWindingOrder(GfxPolygonWindingOrder order)
{
    switch (order)
    {
    case GfxPolygonWindingOrder_CCW: return GL_CCW;
    case GfxPolygonWindingOrder_CW:  return GL_CW;
    }
    Panic("Invalid value for order");
    return 0;
}

GLenum GLBlendFactor(GfxBlendFactor factor)
{
    switch (factor)
    {
    case GfxBlendFactor_One:              return GL_ONE;
    case GfxBlendFactor_Zero:             return GL_ZERO;
    case GfxBlendFactor_SrcColor:         return GL_SRC_COLOR;
    case GfxBlendFactor_OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
    case GfxBlendFactor_SrcAlpha:         return GL_SRC_ALPHA;
    case GfxBlendFactor_OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case GfxBlendFactor_DstColor:         return GL_DST_COLOR;
    case GfxBlendFactor_OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
    case GfxBlendFactor_DstAlpha:         return GL_DST_ALPHA;
    case GfxBlendFactor_OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    Panic("Invalid value for factor");
    return 0;
}

GLenum GLBlendEquation(GfxBlendOperation op)
{
    switch (op)
    {
    case GfxBlendOperation_Add:             return GL_FUNC_ADD;
    case GfxBlendOperation_Subtract:        return GL_FUNC_SUBTRACT;
    case GfxBlendOperation_ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    case GfxBlendOperation_Min:             return GL_MIN;
    case GfxBlendOperation_Max:             return GL_MAX;
    }
    Panic("Invalid value for op");
    return 0;
}

GLenum GLComparisonFunc(GfxCompareFunc func)
{
    switch (func)
    {
    case GfxCompareFunc_Never:        return GL_NEVER;
    case GfxCompareFunc_Less:         return GL_LESS;
    case GfxCompareFunc_LessEqual:    return GL_LEQUAL;
    case GfxCompareFunc_Greater:      return GL_GREATER;
    case GfxCompareFunc_GreaterEqual: return GL_GEQUAL;
    case GfxCompareFunc_Equal:        return GL_EQUAL;
    case GfxCompareFunc_NotEqual:     return GL_NOTEQUAL;
    case GfxCompareFunc_Always:       return GL_ALWAYS;
    }
    Panic("Invalid value for func");
    return 0;
}