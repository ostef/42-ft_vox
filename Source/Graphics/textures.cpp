#include "Graphics/Renderer.hpp"
#include "World.hpp"

#include <stb_image.h>

GfxTexture g_block_atlas;

struct LoadedImage
{
    u32 *pixels = null;
    int width = 0;
    int height = 0;
};

static u32 GetPixel(LoadedImage *img, int x, int y)
{
    if (x < 0 || x >= img->width)
        return 0;
    if (y < 0 || y >= img->height)
        return 0;

    return img->pixels[y * img->width + x];
}

static void SetPixel(LoadedImage *img, int x, int y, u32 px)
{
    if (x < 0 || x >= img->width)
        return;
    if (y < 0 || y >= img->height)
        return;

    img->pixels[y * img->width + x] = px;
}

static void BlitPixels(u32 *dest, Vec3u dest_size, Vec3u dest_origin, LoadedImage *src_img, int border_size)
{
    LoadedImage dst_img = {};
    dst_img.pixels = dest + dest_origin.z * dest_size.x * dest_size.y;
    dst_img.width = dest_size.x;
    dst_img.height = dest_size.y;

    for (int y = -border_size; y < src_img->height + border_size; y += 1)
    {
        for (int x = -border_size; x < src_img->width + border_size; x += 1)
        {
            int sample_x = Clamp(x, 0, src_img->width - 1);
            int sample_y = Clamp(y, 0, src_img->height - 1);

            u32 px = GetPixel(src_img, sample_x, sample_y);
            SetPixel(&dst_img, dest_origin.x + x + border_size, dest_origin.y + y + border_size, px);
        }
    }
}

static Result<LoadedImage> LoadImage(String filename)
{
    int width, height;
    u32 *pixels = (u32 *)stbi_load(CloneToCString(filename, temp), &width, &height, null, 4);
    if (!pixels)
        return Result<LoadedImage>::Bad(false);

    LoadedImage result = {};
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    if (result.width != Block_Texture_Size_No_Border || result.height != Block_Texture_Size_No_Border)
    {
        LogError(Log_Graphics, "Block texture '%.*s' has size %dx%d but we expected %dx%d", FSTR(filename), width, height, Block_Texture_Size_No_Border, Block_Texture_Size_No_Border);

        return Result<LoadedImage>::Bad(false);
    }

    return Result<LoadedImage>::Good(result, true);
}

void LoadBlockAtlasTexture()
{
    u32 *pixels = Alloc<u32>(Block_Atlas_Size * Block_Atlas_Size * 6, heap);
    memset(pixels, 0, Block_Atlas_Size * Block_Atlas_Size * 6 * sizeof(u32));

    for (int i = 1; i < Block_Count; i += 1)
    {
        String name = Block_Infos[i].name;
        String filename = TPrintf("Data/Blocks/%.*s.png", FSTR(name));
        String filename_top = TPrintf("Data/Blocks/%.*s_top.png", FSTR(name));
        String filename_bottom = TPrintf("Data/Blocks/%.*s_bottom.png", FSTR(name));
        String filename_east = TPrintf("Data/Blocks/%.*s_east.png", FSTR(name));
        String filename_west = TPrintf("Data/Blocks/%.*s_west.png", FSTR(name));
        String filename_north = TPrintf("Data/Blocks/%.*s_north.png", FSTR(name));
        String filename_south = TPrintf("Data/Blocks/%.*s_south.png", FSTR(name));

        auto image = LoadImage(filename);
        auto image_top = LoadImage(filename_top);
        auto image_bottom = LoadImage(filename_bottom);
        auto image_east = LoadImage(filename_east);
        auto image_west = LoadImage(filename_west);
        auto image_north = LoadImage(filename_north);
        auto image_south = LoadImage(filename_south);
        if (!image_top.ok || !image_bottom.ok || !image_east.ok || !image_west.ok || !image_north.ok || !image_south.ok)
        {
            Assert(image.ok, "Could not load base image for block '%.*s'", FSTR(name));
        }

        uint block_x = ((i - 1) % Block_Atlas_Num_Blocks) * Block_Texture_Size;
        uint block_y = ((i - 1) / Block_Atlas_Num_Blocks) * Block_Texture_Size;
        if (image_east.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_East}, &image_east.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_East}, &image.value, Block_Texture_Border);
        if (image_west.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_West}, &image_west.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_West}, &image.value, Block_Texture_Border);
        if (image_top.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Top}, &image_top.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Top}, &image.value, Block_Texture_Border);
        if (image_bottom.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Bottom}, &image_bottom.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_Bottom}, &image.value, Block_Texture_Border);
        if (image_north.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_North}, &image_north.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_North}, &image.value, Block_Texture_Border);
        if (image_south.ok)
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_South}, &image_south.value, Block_Texture_Border);
        else
            BlitPixels(pixels, {Block_Atlas_Size, Block_Atlas_Size, 6}, {block_x, block_y, BlockFace_South}, &image.value, Block_Texture_Border);
    }

    GfxTextureDesc desc{};
    desc.type = GfxTextureType_Texture2DArray;
    desc.pixel_format = GfxPixelFormat_RGBAUnorm8;
    desc.width = Block_Atlas_Size;
    desc.height = Block_Atlas_Size;
    desc.array_length = 6;
    desc.num_mipmap_levels = Block_Texture_Border - 1;
    desc.usage = GfxTextureUsage_ShaderRead;
    g_block_atlas = GfxCreateTexture("Block Atlas", desc);
    Assert(!IsNull(&g_block_atlas));

    GfxReplaceTextureRegion(&g_block_atlas, {0,0,0}, {Block_Atlas_Size, Block_Atlas_Size, 6}, 0, 0, pixels);
    QueueGenerateMipmaps(&g_block_atlas);
}

static Array<GfxTexture *> g_mipmaps_to_generate;

void QueueGenerateMipmaps(GfxTexture *texture)
{
    if (!g_mipmaps_to_generate.allocator.func)
        g_mipmaps_to_generate.allocator = heap;

    ArrayPush(&g_mipmaps_to_generate, texture);
}

void GeneratePendingMipmaps(GfxCommandBuffer *cmd_buffer)
{
    auto pass = GfxBeginCopyPass("Generate Mipmaps", cmd_buffer);
    {
        foreach (i, g_mipmaps_to_generate)
            GfxGenerateMipmaps(&pass, g_mipmaps_to_generate[i]);
    }
    GfxEndCopyPass(&pass);

    ArrayClear(&g_mipmaps_to_generate);
}
