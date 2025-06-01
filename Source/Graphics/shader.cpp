#include "Graphics/Renderer.hpp"

static ShaderFile g_shader_files[] = {
    {.name="mesh_geometry"},
    {.name="shadow_map_geometry"},
    {.name="screen_space"},
    {.name="post_processing"},
    {.name="ui"},
    {.name="Sky/transmittance_LUT"},
    {.name="Sky/multi_scatter_LUT"},
    {.name="Sky/color_LUT"},
    {.name="Sky/atmosphere"},
};

static bool LoadShader(ShaderFile *file)
{
    file->stages = 0;

    if (Gfx_Backend == GfxBackend_OpenGL)
    {
        bool has_vertex = false;
        String vert_filename = TPrintf("Shaders/OpenGL/%.*s.vert.glsl", file->name.length, file->name.data);
        if (FileExists(vert_filename))
        {
            has_vertex = true;

            ShaderPreprocessor pp{};
            if (!InitShaderPreprocessor(&pp, vert_filename))
                return false;

            auto pp_result = PreprocessShader(&pp);
            if (!pp_result.ok)
                return false;

            GfxShader shader = GfxLoadShader(file->name, pp_result.source_code, GfxPipelineStage_Vertex);
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

        bool has_fragment = false;
        String frag_filename = TPrintf("Shaders/OpenGL/%.*s.frag.glsl", file->name.length, file->name.data);
        if (FileExists(frag_filename))
        {
            has_fragment = true;

            ShaderPreprocessor pp{};
            if (!InitShaderPreprocessor(&pp, frag_filename))
                return false;

            auto pp_result = PreprocessShader(&pp);
            if (!pp_result.ok)
                return false;

            GfxShader shader = GfxLoadShader(file->name, pp_result.source_code, GfxPipelineStage_Fragment);
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
            LogError(Log_Shaders, "Could not find any shader file for shader '%.*s'", FSTR(file->name));
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

            Panic("Shader '%.*s' has no vertex stage", FSTR(name));
            return null;
        }
    }

    Panic("No shader named '%.*s'", FSTR(name));
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

            Panic("Shader '%.*s' has no fragment stage", FSTR(name));
            return null;
        }
    }

    Panic("No shader named '%.*s'", FSTR(name));
    return null;
}
