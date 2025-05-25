#include "Graphics.hpp"

GfxRenderPass GfxBeginRenderPass(String name, GfxCommandBuffer *cmd_buffer, GfxRenderPassDesc desc) { return {}; }
void GfxEndRenderPass(GfxRenderPass *pass) {}

void GfxSetPipelineState(GfxRenderPass *pass, GfxPipelineState *state) {}
void GfxSetViewport(GfxRenderPass *pass, GfxViewport viewport) {}
void GfxSetScissorRect(GfxRenderPass *pass, Recti rect) {}

void GfxSetVertexBuffer(GfxRenderPass *Pass, int index, GfxBuffer *buffer, s64 offset, s64 size, s64 stride) {}

void GfxSetBuffer(GfxRenderPass *pass, GfxPipelineBinding binding, GfxBuffer *buffer, s64 offset, s64 size) {}
void GfxSetTexture(GfxRenderPass *pass, GfxPipelineBinding binding, GfxTexture *texture) {}
void GfxSetSamplerState(GfxRenderPass *pass, GfxPipelineBinding binding, GfxSamplerState *sampler) {}

void GfxDrawPrimitives(GfxRenderPass *pass, u32 vertex_count, u32 instance_count, u32 base_vertex, u32 base_instance) {}
void GfxDrawIndexedPrimitives(GfxRenderPass *pass, GfxBuffer *index_buffer, u32 index_count, GfxIndexType index_type, u32 instance_count, u32 base_vertex, u32 base_index, u32 base_instance) {}
