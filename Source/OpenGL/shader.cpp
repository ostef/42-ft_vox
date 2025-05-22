#include "Graphics.hpp"

bool IsNull(GfxShader *shader)
{
    return shader == null || shader->handle == 0;
}

static Slice<GfxPipelineBinding> GetGLShaderBindings(String name, GLuint handle, GfxPipelineStage stage)
{
    Array<GfxPipelineBinding> bindings{};
    bindings.allocator = heap;

    int num_uniform_buffers = 0;
    int num_storage_buffers = 0;
    int num_uniforms = 0;
    glGetProgramInterfaceiv(handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_uniform_buffers);
    glGetProgramInterfaceiv(handle, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &num_storage_buffers);
    glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

    for (int ty = 0; ty < 2; ty += 1)
    {
        GfxPipelineBindingType type = ty == 0 ? GfxPipelineBindingType_UniformBuffer : GfxPipelineBindingType_StorageBuffer;
        GLenum gl_type = type == GfxPipelineBindingType_UniformBuffer ? GL_UNIFORM_BLOCK : GL_SHADER_STORAGE_BLOCK;
        int num = type == GfxPipelineBindingType_UniformBuffer ? num_uniform_buffers : num_storage_buffers;

        for (int i = 0; i < num; i += 1)
        {
            GLenum props[] = {GL_BUFFER_BINDING, GL_NAME_LENGTH};
            int values[StaticArraySize(props)];
            glGetProgramResourceiv(handle, gl_type, i, StaticArraySize(props), props, sizeof(values), null, values);

            char *name = Alloc<char>(values[1] + 1, heap);
            glGetProgramResourceName(handle, gl_type, i, values[1] + 1, null, name);

            GfxPipelineBinding binding{};
            binding.name = String{values[1], name};
            binding.type = type;
            binding.index = values[0];
            ArrayPush(&bindings, binding);
        }
    }

    for (int i = 0; i < num_uniforms; i += 1)
    {
        GLenum props[] = {GL_LOCATION, GL_NAME_LENGTH, GL_TYPE};
        int values[StaticArraySize(props)];
        glGetProgramResourceiv(handle, GL_UNIFORM, i, StaticArraySize(props), props, sizeof(values), null, values);

        if (values[0] < 0)
            continue;

        bool is_sampler = false;
        for (int j = 0; j < (int)StaticArraySize(GL_Sampler_Types); j += 1)
        {
            if (values[2] == (int)GL_Sampler_Types[j])
            {
                is_sampler = true;
                break;
            }
        }

        if (!is_sampler)
            continue;

        char *name = Alloc<char>(values[1] + 1, heap);
        glGetProgramResourceName(handle, GL_UNIFORM, i, values[1] + 1, null, name);

        int binding_index = -1;
        glGetUniformiv(handle, values[0], &binding_index);

        GfxPipelineBinding binding{};
        binding.name = String{values[1], name};
        binding.type = GfxPipelineBindingType_CombinedSampler;
        binding.index = binding_index;
        ArrayPush(&bindings, binding);
    }

    return {.count=bindings.count, .data=bindings.data};
}

GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage, Slice<GfxPipelineBinding> bindings)
{
    const char *stage_name = stage == GfxPipelineStage_Vertex ? "vertex" : "fragment";
    GLenum gl_stage = stage == GfxPipelineStage_Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    char *c_source = CloneToCString(source_code, temp);

    GLuint handle = glCreateShaderProgramv(gl_stage, 1, &c_source);

    int status;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);

    if (!status)
    {
        int log_length;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);

        char *buffer = Alloc<char>(log_length + 1, temp);
        glGetProgramInfoLog(handle, log_length, null, buffer);
        LogError(Log_OpenGL, "Failed to compile and link %s shader '%.*s':\n\n%s", stage_name, name.length, name.data, buffer);

        glDeleteProgram(handle);

        return {};
    }

    String program_name = TPrintf("Shader '%.*s' (%s)", name.length, name.data, stage_name);
    glObjectLabel(GL_PROGRAM, handle, program_name.length, program_name.data);

    GfxShader result{};
    result.handle = handle;
    result.stage = stage;
    result.bindings = bindings;

    return result;
}

GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage)
{
    GfxShader shader = GfxLoadShader(name, source_code, stage, {});
    if (IsNull(&shader))
        return shader;

    shader.bindings = GetGLShaderBindings(name, shader.handle, stage);

    return shader;
}

void GfxDestroyShader(GfxShader *shader)
{
    glDeleteProgram(shader->handle);

    foreach (i, shader->bindings)
    {
        Free(shader->bindings[i].name.data, heap);
        Free(shader->bindings[i].associated_texture_units.data, heap);
    }

    Free(shader->bindings.data, heap);

    *shader = {};
}
