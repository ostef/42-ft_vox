#include "Renderer.hpp"

static ShaderFile g_shader_files[] = {
    {.name="mesh_geometry"},
};

static bool LoadShader(ShaderFile *file)
{
    file->stages = 0;

    if (Gfx_Backend == GfxBackend_OpenGL)
    {
        bool has_vertex = false;
        bool has_fragment = false;
        String vert_filename = TPrintf("Shaders/OpenGL/%.*s.vert.glsl", file->name.length, file->name.data);
        if (FileExists(vert_filename))
        {
            has_vertex = true;
            auto result = ReadEntireFile(vert_filename);
            if (!result.ok)
            {
                LogError(Log_Shaders, "Could not read file '%.*s'", vert_filename.length, vert_filename.data);
                return false;
            }

            String source_code = result.value;
            GfxShader shader = GfxLoadShader(file->name, source_code, GfxPipelineStage_Vertex, {});
            if (IsNull(&shader))
                return false;

            GfxDestroyShader(&file->vertex_shader);
            file->vertex_shader = shader;
            file->stages |= GfxPipelineStageFlag_Vertex;
        }
        else
        {
            GfxDestroyShader(&file->vertex_shader);
        }

        String frag_filename = TPrintf("Shaders/OpenGL/%.*s.frag.glsl", file->name.length, file->name.data);
        if (FileExists(frag_filename))
        {
            has_fragment = true;
            auto result = ReadEntireFile(frag_filename);
            if (!result.ok)
            {
                LogError(Log_Shaders, "Could not read file '%.*s'", frag_filename.length, frag_filename.data);
                return false;
            }

            String source_code = result.value;
            GfxShader shader = GfxLoadShader(file->name, source_code, GfxPipelineStage_Fragment, {});
            if (IsNull(&shader))
                return false;

            GfxDestroyShader(&file->fragment_shader);
            file->fragment_shader = shader;
            file->stages |= GfxPipelineStageFlag_Fragment;
        }
        else
        {
            GfxDestroyShader(&file->fragment_shader);
        }

        if (!has_vertex && !has_fragment)
        {
            LogError(Log_Shaders, "Could not find any shader file for shader '%.*s'", file->name.length, file->name.data);
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

void LoadAllShaders()
{
    bool ok = true;
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        ok &= LoadShader(&g_shader_files[i]);
    }

    if (!ok)
    {
        LogError(Log_Shaders, "There were errors when loading shaders. Exiting.");
        exit(1);
    }
}

GfxShader *GetVertexShader(String name)
{
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        if (g_shader_files[i].name == name)
        {
            if (g_shader_files[i].stages & GfxPipelineStageFlag_Vertex)
                return &g_shader_files[i].vertex_shader;

            Panic("Shader '%.*s' has no vertex stage", name.length, name.data);
            return null;
        }
    }

    Panic("No shader named '%.*s'", name.length, name.data);
    return null;
}

GfxShader *GetFragmentShader(String name)
{
    for (int i = 0; i < (int)StaticArraySize(g_shader_files); i += 1)
    {
        if (g_shader_files[i].name == name)
        {
            if (g_shader_files[i].stages & GfxPipelineStageFlag_Fragment)
                return &g_shader_files[i].fragment_shader;

            Panic("Shader '%.*s' has no fragment stage", name.length, name.data);
            return null;
        }
    }

    Panic("No shader named '%.*s'", name.length, name.data);
    return null;
}
