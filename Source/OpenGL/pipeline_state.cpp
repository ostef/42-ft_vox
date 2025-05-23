#include "Graphics.hpp"

static void SetupPipelineBindings(GfxPipelineState *state);

bool IsNull(GfxPipelineState *state)
{
    return state == null || state->pso == 0 || state->vao == 0;
}

GfxPipelineStateDesc GetDesc(GfxPipelineState *state)
{
    return state->desc;
}

GfxPipelineState GfxCreatePipelineState(String name, GfxPipelineStateDesc desc)
{
    if (!desc.vertex_shader)
        return {};

    GfxPipelineState result{};
    result.desc = desc;

    glGenProgramPipelines(1, &result.pso);

    if (desc.vertex_shader)
        glUseProgramStages(result.pso, GL_VERTEX_SHADER_BIT, desc.vertex_shader->handle);
    if (desc.fragment_shader)
        glUseProgramStages(result.pso, GL_FRAGMENT_SHADER_BIT, desc.fragment_shader->handle);

    String pso_name = TPrintf("Pipeline '%.*s'", name.length, name.data);
    glObjectLabel(GL_PROGRAM_PIPELINE, result.pso, pso_name.length, pso_name.data);

    glCreateVertexArrays(1, &result.vao);
    String vao_name = TPrintf("Vertex Array '%.*s'", name.length, name.data);
    glObjectLabel(GL_VERTEX_ARRAY, result.vao, vao_name.length, vao_name.data);

    foreach (i, desc.vertex_layout)
    {
        auto layout = desc.vertex_layout[i];
        glEnableVertexArrayAttrib(result.vao, i);

        int size;
        GLenum type;
        GLFormatSizeAndType(layout.format, &size, &type);
        Assert(size != 0 && type != GL_NONE);

        if (type == GL_FLOAT || layout.normalized)
            glVertexArrayAttribFormat(result.vao, i, size, type, layout.normalized, layout.offset);
        else
            glVertexArrayAttribIFormat(result.vao, i, size, type, layout.offset);

        glVertexArrayAttribBinding(result.vao, i, layout.buffer_index);
    }

    SetupPipelineBindings(&result);

    return result;
}

void GfxDestroyPipelineState(GfxPipelineState *state)
{
    foreach (i, state->vertex_stage_bindings)
    {
        Free(state->vertex_stage_bindings[i].name.data, heap);
        Free(state->vertex_stage_bindings[i].associated_texture_units.data, heap);
    }

    foreach (i, state->fragment_stage_bindings)
    {
        Free(state->fragment_stage_bindings[i].name.data, heap);
        Free(state->fragment_stage_bindings[i].associated_texture_units.data, heap);
    }

    Free(state->vertex_stage_bindings.data, heap);
    Free(state->fragment_stage_bindings.data, heap);

    glDeleteProgramPipelines(1, &state->pso);
    glDeleteVertexArrays(1, &state->vao);
    *state = {};
}

Slice<GfxPipelineBinding> GfxGetVertexStageBindings(GfxPipelineState *state)
{
    return state->vertex_stage_bindings;
}

Slice<GfxPipelineBinding> GfxGetFragmentStageBindings(GfxPipelineState *state)
{
    return state->fragment_stage_bindings;
}

void GLFormatSizeAndType(GfxVertexFormat format, int *size, GLenum *type)
{
    switch (format)
    {
    case GfxVertexFormat_Float:
        *size = 1;
        *type = GL_FLOAT;
        return;

    case GfxVertexFormat_Float2:
        *size = 2;
        *type = GL_FLOAT;
        return;

    case GfxVertexFormat_Float3:
        *size = 3;
        *type = GL_FLOAT;
        return;

    case GfxVertexFormat_Float4:
        *size = 4;
        *type = GL_FLOAT;
        return;
    }

    *size = 0;
    *type = GL_NONE;
    return;
}

// When using separable programs, binding points are assigned without taking into account the other stages, so
// the same binding point can be assigned to different buffers.
// This function reassigns the binding point for the fragment shader in a pipeline state in such a way that
// bindings do not overlap. This must be called every time a pipeline state is bound, and is of course an
// inefficient and horrible
void SetupPipelineBindings(GfxPipelineState *state)
{
    state->vertex_stage_bindings = GfxCloneBindings(state->desc.vertex_shader->bindings);

    if (!state->desc.fragment_shader)
        return;

    state->fragment_stage_bindings = GfxCloneBindings(state->desc.fragment_shader->bindings);
    state->fragment_stage_binding_relocations = AllocSlice<OpenGLBindingRelocation>(state->fragment_stage_bindings.count, heap, true);

    int first_available_fragment_buffer_binding = 0;
    int first_available_fragment_sampler_binding = 0;
    foreach (i, state->vertex_stage_bindings)
    {
        auto b = state->vertex_stage_bindings[i];
        switch (b.type)
        {
        case GfxPipelineBindingType_UniformBuffer:
        case GfxPipelineBindingType_StorageBuffer: {
            first_available_fragment_buffer_binding = Max(first_available_fragment_buffer_binding, b.index + 1);
        } break;

        case GfxPipelineBindingType_Texture:
        case GfxPipelineBindingType_SamplerState:
        case GfxPipelineBindingType_CombinedSampler: {
            if (b.associated_texture_units.count > 0)
            {
                foreach (u, b.associated_texture_units)
                    first_available_fragment_sampler_binding = Max(first_available_fragment_sampler_binding, b.associated_texture_units[u] + 1);
            }
            else
            {
                first_available_fragment_sampler_binding = Max(first_available_fragment_sampler_binding, b.index + 1);
            }

        } break;
        }
    }

    int num_uniform_buffers = 0;
    int num_storage_buffers = 0;
    int num_uniforms = 0;
    glGetProgramInterfaceiv(state->desc.fragment_shader->handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_uniform_buffers);
    glGetProgramInterfaceiv(state->desc.fragment_shader->handle, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &num_storage_buffers);
    glGetProgramInterfaceiv(state->desc.fragment_shader->handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

    foreach (i, state->fragment_stage_bindings)
    {
        auto b = &state->fragment_stage_bindings[i];
        auto reloc = &state->fragment_stage_binding_relocations[i];
        switch (b->type)
        {
        case GfxPipelineBindingType_UniformBuffer:
        case GfxPipelineBindingType_StorageBuffer: {
            if (b->index < 0)
                continue;

            reloc->original_binding_index = b->index;
            reloc->relocated_binding_index = b->index + first_available_fragment_buffer_binding;
            b->index += first_available_fragment_buffer_binding;

            GLenum gl_type = b->type == GfxPipelineBindingType_UniformBuffer ? GL_UNIFORM_BLOCK : GL_SHADER_STORAGE_BLOCK;
            int num = b->type == GfxPipelineBindingType_UniformBuffer ? num_uniform_buffers : num_storage_buffers;
            for (int res_index = 0; res_index < num; res_index += 1)
            {
                GLenum props[] = {GL_BUFFER_BINDING};
                int values[StaticArraySize(props)];
                glGetProgramResourceiv(state->desc.fragment_shader->handle, gl_type, res_index, StaticArraySize(props), props, sizeof(values), null, values);

                if (values[0] == reloc->original_binding_index)
                {
                    reloc->block_index_or_uniform_location = res_index;
                    break;
                }
            }
        } break;

        case GfxPipelineBindingType_Texture:
        case GfxPipelineBindingType_SamplerState: {
            reloc->associated_texture_units = AllocSlice<OpenGLBindingRelocation>(b->associated_texture_units.count, heap, true);
            foreach (unit_index, b->associated_texture_units)
            {
                auto unit = &b->associated_texture_units[unit_index];
                auto reloc_unit = &reloc->associated_texture_units[unit_index];

                reloc_unit->original_binding_index = b->index;
                reloc_unit->relocated_binding_index = b->index + first_available_fragment_sampler_binding;
                b->index += first_available_fragment_sampler_binding;

                for (int res_index = 0; res_index < num_uniforms; res_index += 1)
                {
                    GLenum props[] = {GL_LOCATION, GL_TYPE};
                    int values[StaticArraySize(props)];
                    glGetProgramResourceiv(state->desc.fragment_shader->handle, GL_UNIFORM, res_index, StaticArraySize(props), props, sizeof(values), null, values);

                    if (values[0] < 0)
                        continue;

                    bool is_sampler = false;
                    for (int s = 0; s < (int)StaticArraySize(GL_Sampler_Types); s += 1)
                    {
                        if (values[1] == (int)GL_Sampler_Types[s])
                        {
                            is_sampler = true;
                            break;
                        }
                    }

                    if (!is_sampler)
                        continue;

                    int binding_index = -1;
                    glGetUniformiv(state->desc.fragment_shader->handle, values[0], &binding_index);

                    if (binding_index == reloc_unit->original_binding_index)
                    {
                        reloc_unit->block_index_or_uniform_location = values[0];
                        reloc_unit->original_binding_index = *unit;
                        reloc_unit->relocated_binding_index = *unit + first_available_fragment_sampler_binding;
                        break;
                    }
                }

                *unit += first_available_fragment_sampler_binding;
            }
        } break;

        case GfxPipelineBindingType_CombinedSampler: {
            if (b->index < 0)
                continue;

            reloc->original_binding_index = b->index;
            reloc->relocated_binding_index = b->index + first_available_fragment_sampler_binding;
            b->index += first_available_fragment_sampler_binding;

            for (int res_index = 0; res_index < num_uniforms; res_index += 1)
            {
                GLenum props[] = {GL_LOCATION, GL_TYPE};
                int values[StaticArraySize(props)];
                glGetProgramResourceiv(state->desc.fragment_shader->handle, GL_UNIFORM, res_index, StaticArraySize(props), props, sizeof(values), null, values);

                if (values[0] < 0)
                    continue;

                bool is_sampler = false;
                for (int s = 0; s < (int)StaticArraySize(GL_Sampler_Types); s += 1)
                {
                    if (values[1] == (int)GL_Sampler_Types[s])
                    {
                        is_sampler = true;
                        break;
                    }
                }

                if (!is_sampler)
                    continue;

                int binding_index = -1;
                glGetUniformiv(state->desc.fragment_shader->handle, values[0], &binding_index);

                if (binding_index == reloc->original_binding_index)
                {
                    reloc->block_index_or_uniform_location = values[0];
                    break;
                }
            }
        } break;
        }
    }
}
