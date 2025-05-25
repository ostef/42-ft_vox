#include "Graphics.hpp"

bool IsNull(GfxShader *shader)
{
    return shader == null || shader->library == null || shader->function == null;
}

GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage, Slice<GfxPipelineBinding> bindings) { return {}; }
GfxShader GfxLoadShader(String name, String source_code, GfxPipelineStage stage) { return {}; }
void GfxDestroyShader(GfxShader *shader) {}
