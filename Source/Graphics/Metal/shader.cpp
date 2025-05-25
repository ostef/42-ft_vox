#include "Graphics.hpp"

bool IsNull(GfxShader *shader)
{
    return shader == null || shader->library == null || shader->function == null;
}

GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage, Slice<GfxPipelineBinding> bindings) { return {}; }
GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage) { return {}; }

void GfxDestroyShader(GfxShader *shader)
{
    shader->library->release();
    shader->function->release();

    foreach (i, shader->bindings)
    {
        Free(shader->bindings[i].name.data, heap);
        Free(shader->bindings[i].associated_texture_units.data, heap);
    }

    Free(shader->bindings.data, heap);

    *shader = {};
}
