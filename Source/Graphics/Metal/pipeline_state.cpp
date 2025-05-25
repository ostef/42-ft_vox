#include "Graphics.hpp"

bool IsNull(GfxPipelineState *state)
{
    return state == null || state->handle == null;
}

GfxPipelineStateDesc GetDesc(GfxPipelineState *state)
{
    return state->desc;
}

GfxPipelineState GfxCreatePipelineState(String name, GfxPipelineStateDesc desc) { return {}; }
void GfxDestroyPipelineState(GfxPipelineState *state) {}

Slice<GfxPipelineBinding> GfxGetVertexStageBindings(GfxPipelineState *state)
{
    return state->vertex_stage_bindings;
}

Slice<GfxPipelineBinding> GfxGetFragmentStageBindings(GfxPipelineState *state)
{
    return state->fragment_stage_bindings;
}
