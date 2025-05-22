#include "Graphics.hpp"

static GLuint GLGetFramebuffer(GfxRenderPassDesc desc)
{
    OpenGLFramebufferKey key{};
    bool has_swapchain_texture = false;
    for (int i = 0; i < (int)StaticArraySize(desc.color_attachments); i += 1)
    {
        auto texture = desc.color_attachments[i];
        if (texture == GfxGetSwapchainTexture())
        {
            Assert(i == 0, "Swapchain texture can only be used as the only texture for attachment 0");
            has_swapchain_texture = true;
        }

        key.color_textures[i] = texture ? texture->handle : 0;
    }

    if (has_swapchain_texture)
        Assert(desc.depth_attachment == null && desc.stencil_attachment == null, "Swapchain texture can only be used as the only texture for attachment 0");

    key.depth_texture = desc.depth_attachment ? desc.depth_attachment->handle : 0;
    key.stencil_texture = desc.stencil_attachment ? desc.stencil_attachment->handle : 0;

    if (has_swapchain_texture)
        return 0; // Return default framebuffer

    bool exists = false;
    auto ptr = HashMapFindOrAdd(&g_gfx_context.framebuffer_cache, key, &exists);
    if (exists)
        return *ptr;

    glCreateFramebuffers(1, ptr);

    GLenum draw_buffers[Gfx_Max_Color_Attachments] = {};
    int num_draw_buffers = 0;

    for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
    {
        if (!key.color_textures[i])
            continue;

        glNamedFramebufferTexture(
            *ptr,
            GL_COLOR_ATTACHMENT0 + i,
            key.color_textures[i],
            0
        );

        draw_buffers[num_draw_buffers] = GL_COLOR_ATTACHMENT0 + i;
        num_draw_buffers += 1;
    }

    glNamedFramebufferDrawBuffers(*ptr, num_draw_buffers, draw_buffers);

    if (key.depth_texture)
    {
        glNamedFramebufferTexture(
            *ptr,
            GL_DEPTH_ATTACHMENT,
            key.depth_texture,
            0
        );
    }

    if (key.stencil_texture)
    {
        glNamedFramebufferTexture(
            *ptr,
            GL_STENCIL_ATTACHMENT,
            key.stencil_texture,
            0
        );
    }

    auto status = glCheckNamedFramebufferStatus(*ptr, GL_DRAW_FRAMEBUFFER);
    Assert(status == GL_FRAMEBUFFER_COMPLETE, "Failed to create framebuffer: %x", status);

    return *ptr;
}

GfxRenderPassDesc GetDesc(GfxRenderPass *pass)
{
    return pass->desc;
}

GfxRenderPass GfxBeginRenderPass(String name, GfxCommandBuffer *cmd_buffer, GfxRenderPassDesc desc)
{
    GfxBeginDebugGroup(cmd_buffer, name);

    GfxRenderPass pass {};
    pass.name = name;
    pass.desc = desc;
    pass.cmd_buffer = cmd_buffer;

    pass.fbo = GLGetFramebuffer(desc);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass.fbo);

    glDisable(GL_SCISSOR_TEST); // Prevent scissor from affecting clearing

    int color_buffer_index = 0;
    for (int i = 0; i < Gfx_Max_Color_Attachments; i += 1)
    {
        if (desc.color_attachments[i])
        {
            glColorMaski(color_buffer_index, true, true, true, true);
            if (desc.should_clear_color[i])
                glClearBufferfv(GL_COLOR, color_buffer_index, (float *)&desc.clear_color[i]);

            color_buffer_index += 1;
        }
    }

    if (desc.depth_attachment && desc.should_clear_depth)
    {
        glDepthMask(true);
        glClearBufferfv(GL_DEPTH, 0, &desc.clear_depth);
    }

    if (desc.stencil_attachment && desc.should_clear_stencil)
    {
        glStencilMask(0xffffffff);
        glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glClearBufferiv(GL_STENCIL, 0, (int *)&desc.clear_stencil);
    }

    return pass;
}

void GfxEndRenderPass(GfxRenderPass *pass)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    GfxEndDebugGroup(pass->cmd_buffer);
}

