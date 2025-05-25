#include "Graphics.hpp"

bool IsNull(GfxTexture *texture)
{
    return texture == null || texture->handle == null;
}

GfxTextureDesc GetDesc(GfxTexture *texture)
{
    return texture->desc;
}

GfxTexture GfxCreateTexture(String name, GfxTextureDesc desc) { return {}; }
void GfxDestroyTexture(GfxTexture *texture) {}
void GfxReplaceTextureRegion(GfxTexture *texture, Vec3u origin, Vec3u size, u32 mipmap_level, u32 array_slice, const void *bytes) {}

bool IsNull(GfxSamplerState *sampler)
{
    return sampler == null || sampler->handle == null;
}

GfxSamplerStateDesc GetDesc(GfxSamplerState *sampler)
{
    return sampler->desc;
}

GfxSamplerState GfxCreateSamplerState(String name, GfxSamplerStateDesc desc) { return {}; }
void GfxDestroySamplerState(GfxSamplerState *sampler) {}

