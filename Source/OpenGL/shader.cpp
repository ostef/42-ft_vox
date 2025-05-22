#include "Graphics.hpp"

bool IsNull(GfxShader *shader)
{
    return shader == null || shader->handle == 0;
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
        LogError(Log_OpenGL, "Failed to compile and link %s shader:\n\n%s", stage_name, buffer);

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

void GfxDestroyShader(GfxShader *shader)
{
    glDeleteProgram(shader->handle);

    foreach(i, shader->bindings)
    {
        Free(shader->bindings[i].name.data, heap);
        Free(shader->bindings[i].associated_texture_units.data, heap);
    }

    Free(shader->bindings.data, heap);

    *shader = {};
}
