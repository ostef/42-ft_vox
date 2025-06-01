#include "Graphics/Renderer.hpp"

SkyAtmosphere g_sky;

GfxPipelineState g_sky_transmittance_LUT_pipeline;
GfxPipelineState g_sky_multi_scatter_LUT_pipeline;
GfxPipelineState g_sky_color_LUT_pipeline;
GfxPipelineState g_sky_atmosphere_pipeline;
GfxSamplerState g_sky_LUT_sampler;
GfxSamplerState g_sky_color_LUT_sampler;

static void InitSkyPipelines()
{
    GfxPipelineStateDesc desc = {};
    desc.vertex_shader = GetVertexShader("screen_space");
    desc.fragment_shader = GetFragmentShader("Sky/transmittance_LUT");
    desc.color_formats[0] = GfxPixelFormat_RGBAFloat32;
    g_sky_transmittance_LUT_pipeline = GfxCreatePipelineState("Sky Transmittance LUT", desc);

    desc.fragment_shader = GetFragmentShader("Sky/multi_scatter_LUT");
    g_sky_multi_scatter_LUT_pipeline = GfxCreatePipelineState("Sky Multi Scatter LUT", desc);

    desc.fragment_shader = GetFragmentShader("Sky/color_LUT");
    g_sky_color_LUT_pipeline = GfxCreatePipelineState("Sky Color LUT", desc);

    {
        GfxPipelineStateDesc desc = {};
        desc.vertex_shader = GetVertexShader("Sky/atmosphere");
        desc.fragment_shader = GetFragmentShader("Sky/atmosphere");
        desc.color_formats[0] = GfxPixelFormat_RGBAFloat32;
        desc.depth_format = GfxPixelFormat_DepthFloat32;
        desc.depth_state = {.enabled=true};
        g_sky_atmosphere_pipeline = GfxCreatePipelineState("Sky Atmosphere", desc);
    }

    {
        GfxSamplerStateDesc sampler_desc = {};
        sampler_desc.min_filter = GfxSamplerFilter_Linear;
        sampler_desc.mag_filter = GfxSamplerFilter_Linear;
        sampler_desc.u_address_mode = GfxSamplerAddressMode_ClampToEdge;
        sampler_desc.v_address_mode = GfxSamplerAddressMode_ClampToEdge;
        g_sky_LUT_sampler = GfxCreateSamplerState("Sky LUT", sampler_desc);
    }
    {
        GfxSamplerStateDesc sampler_desc = {};
        sampler_desc.min_filter = GfxSamplerFilter_Linear;
        sampler_desc.mag_filter = GfxSamplerFilter_Linear;
        sampler_desc.u_address_mode = GfxSamplerAddressMode_Repeat;
        sampler_desc.v_address_mode = GfxSamplerAddressMode_ClampToEdge;
        g_sky_color_LUT_sampler = GfxCreateSamplerState("Sky Color LUT", sampler_desc);
    }
}

static void SkyTransmittanceLUTPass(GfxCommandBuffer *cmd_buffer, s64 sky_offset)
{
    if (IsNull(&g_sky.transmittance_LUT))
    {
        GfxTextureDesc desc = {};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAFloat32;
        desc.width = g_sky.transmittance_LUT_resolution.x;
        desc.height = g_sky.transmittance_LUT_resolution.y;
        desc.usage = GfxTextureUsage_ShaderRead | GfxTextureUsage_RenderTarget;
        g_sky.transmittance_LUT = GfxCreateTexture("Sky Transmittance LUT", desc);
    }

    GfxRenderPassDesc pass_desc = {};
    GfxSetColorAttachment(&pass_desc, 0, &g_sky.transmittance_LUT);

    auto pass = GfxBeginRenderPass("Sky Transmittance LUT", cmd_buffer, pass_desc);
    {
        GfxSetPipelineState(&pass, &g_sky_transmittance_LUT_pipeline);
        GfxSetViewport(&pass, {.width=(float)g_sky.transmittance_LUT_resolution.x, .height=(float)g_sky.transmittance_LUT_resolution.y});

        auto fragment_sky_buffer = GfxGetFragmentStageBinding(&g_sky_transmittance_LUT_pipeline, "sky_buffer");
        GfxSetBuffer(&pass, fragment_sky_buffer, FrameDataBuffer(), sky_offset, sizeof(Std140SkyAtmosphere));

        GfxDrawPrimitives(&pass, 6, 1);
    }
    GfxEndRenderPass(&pass);
}

static void SkyMultiScatterLUTPass(GfxCommandBuffer *cmd_buffer, s64 sky_offset)
{
    if (IsNull(&g_sky.multi_scatter_LUT))
    {
        GfxTextureDesc desc = {};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAFloat32;
        desc.width = g_sky.multi_scatter_LUT_resolution.x;
        desc.height = g_sky.multi_scatter_LUT_resolution.y;
        desc.usage = GfxTextureUsage_ShaderRead | GfxTextureUsage_RenderTarget;
        g_sky.multi_scatter_LUT = GfxCreateTexture("Sky Multi Scatter LUT", desc);
    }

    GfxRenderPassDesc pass_desc = {};
    GfxSetColorAttachment(&pass_desc, 0, &g_sky.multi_scatter_LUT);

    auto pass = GfxBeginRenderPass("Sky Multi Scatter LUT", cmd_buffer, pass_desc);
    {
        GfxSetPipelineState(&pass, &g_sky_multi_scatter_LUT_pipeline);
        GfxSetViewport(&pass, {.width=(float)g_sky.multi_scatter_LUT_resolution.x, .height=(float)g_sky.multi_scatter_LUT_resolution.y});

        auto fragment_sky_buffer = GfxGetFragmentStageBinding(&g_sky_multi_scatter_LUT_pipeline, "sky_buffer");
        auto fragment_transmittance_LUT = GfxGetFragmentStageBinding(&g_sky_multi_scatter_LUT_pipeline, "transmittance_LUT");

        GfxSetBuffer(&pass, fragment_sky_buffer, FrameDataBuffer(), sky_offset, sizeof(Std140SkyAtmosphere));
        GfxSetTexture(&pass, fragment_transmittance_LUT, &g_sky.transmittance_LUT);
        GfxSetSamplerState(&pass, fragment_transmittance_LUT, &g_sky_LUT_sampler);

        GfxDrawPrimitives(&pass, 6, 1);
    }
    GfxEndRenderPass(&pass);
}

static void SkyColorLUTPass(FrameRenderContext *ctx)
{
    if (IsNull(&g_sky.color_LUT))
    {
        GfxTextureDesc desc = {};
        desc.type = GfxTextureType_Texture2D;
        desc.pixel_format = GfxPixelFormat_RGBAFloat32;
        desc.width = g_sky.color_LUT_resolution.x;
        desc.height = g_sky.color_LUT_resolution.y;
        desc.usage = GfxTextureUsage_ShaderRead | GfxTextureUsage_RenderTarget;
        g_sky.color_LUT = GfxCreateTexture("Sky Color LUT", desc);
    }

    GfxRenderPassDesc pass_desc = {};
    GfxSetColorAttachment(&pass_desc, 0, &g_sky.color_LUT);

    auto pass = GfxBeginRenderPass("Sky Color LUT", ctx->cmd_buffer, pass_desc);
    {
        GfxSetPipelineState(&pass, &g_sky_color_LUT_pipeline);
        GfxSetViewport(&pass, {.width=(float)g_sky.color_LUT_resolution.x, .height=(float)g_sky.color_LUT_resolution.y});

        auto fragment_frame_info_buffer = GfxGetFragmentStageBinding(&g_sky_color_LUT_pipeline, "frame_info_buffer");
        auto fragment_transmittance_LUT = GfxGetFragmentStageBinding(&g_sky_color_LUT_pipeline, "transmittance_LUT");
        auto fragment_multi_scatter_LUT = GfxGetFragmentStageBinding(&g_sky_color_LUT_pipeline, "multi_scatter_LUT");

        GfxSetBuffer(&pass, fragment_frame_info_buffer, FrameDataBuffer(), ctx->frame_info_offset, sizeof(Std140FrameInfo));
        GfxSetTexture(&pass, fragment_transmittance_LUT, &g_sky.transmittance_LUT);
        GfxSetSamplerState(&pass, fragment_transmittance_LUT, &g_sky_LUT_sampler);
        GfxSetTexture(&pass, fragment_multi_scatter_LUT, &g_sky.multi_scatter_LUT);
        GfxSetSamplerState(&pass, fragment_multi_scatter_LUT, &g_sky_LUT_sampler);

        GfxDrawPrimitives(&pass, 6, 1);
    }
    GfxEndRenderPass(&pass);
}

void RenderSkyLUTs(FrameRenderContext *ctx)
{
    if (IsNull(&g_sky_transmittance_LUT_pipeline))
        InitSkyPipelines();

    auto sky = Alloc<Std140SkyAtmosphere>(FrameDataAllocator());
    *sky={
        .transmittance_LUT_resolution=g_sky.transmittance_LUT_resolution,
        .multi_scatter_LUT_resolution=g_sky.multi_scatter_LUT_resolution,
        .color_LUT_resolution=g_sky.color_LUT_resolution,
        .num_transmittance_steps=g_sky.num_transmittance_steps,
        .num_multi_scatter_steps=g_sky.num_multi_scatter_steps,
        .num_color_scattering_steps=g_sky.num_color_scattering_steps,
        .rayleigh_scattering_coeff=g_sky.rayleigh_scattering_coeff,
        .rayleigh_absorption_coeff=g_sky.rayleigh_absorption_coeff,
        .mie_scattering_coeff=g_sky.mie_scattering_coeff,
        .mie_absorption_coeff=g_sky.mie_absorption_coeff,
        .mie_asymmetry_value=g_sky.mie_asymmetry_value,
        .ozone_absorption_coeff=g_sky.ozone_absorption_coeff,
        .ground_albedo=g_sky.ground_albedo,
        .ground_level=g_sky.ground_level,
        .ground_radius=g_sky.ground_radius,
        .atmosphere_radius=g_sky.atmosphere_radius,
    };
    s64 sky_offset = GetBufferOffset(FrameDataGfxAllocator(), sky);

    SkyTransmittanceLUTPass(ctx->cmd_buffer, sky_offset);
    SkyMultiScatterLUTPass(ctx->cmd_buffer, sky_offset);
    SkyColorLUTPass(ctx);
}

void SkyAtmospherePass(FrameRenderContext *ctx)
{
    RenderSkyLUTs(ctx);

    int window_w, window_h;
    SDL_GetWindowSizeInPixels(g_window, &window_w, &window_h);

    GfxRenderPassDesc pass_desc = {};
    GfxSetColorAttachment(&pass_desc, 0, &g_main_color_texture);
    GfxSetDepthAttachment(&pass_desc, &g_main_depth_texture);

    auto pass = GfxBeginRenderPass("Sky Atmosphere", ctx->cmd_buffer, pass_desc);
    {
        GfxSetPipelineState(&pass, &g_sky_atmosphere_pipeline);
        GfxSetViewport(&pass, {.width=(float)window_w, .height=(float)window_h});

        auto fragment_frame_info_buffer = GfxGetFragmentStageBinding(&g_sky_atmosphere_pipeline, "frame_info_buffer");
        auto fragment_transmittance_LUT = GfxGetFragmentStageBinding(&g_sky_atmosphere_pipeline, "transmittance_LUT");
        auto fragment_color_LUT = GfxGetFragmentStageBinding(&g_sky_atmosphere_pipeline, "color_LUT");

        GfxSetBuffer(&pass, fragment_frame_info_buffer, FrameDataBuffer(), ctx->frame_info_offset, sizeof(Std140FrameInfo));
        GfxSetTexture(&pass, fragment_transmittance_LUT, &g_sky.transmittance_LUT);
        GfxSetSamplerState(&pass, fragment_transmittance_LUT, &g_sky_LUT_sampler);
        GfxSetTexture(&pass, fragment_color_LUT, &g_sky.color_LUT);
        GfxSetSamplerState(&pass, fragment_color_LUT, &g_sky_color_LUT_sampler);

        GfxDrawPrimitives(&pass, 6, 1);
    }
    GfxEndRenderPass(&pass);
}