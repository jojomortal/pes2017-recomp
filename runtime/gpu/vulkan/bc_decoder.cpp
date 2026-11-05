#include "bc_decoder.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../../env.h"
#include "../../log.h"

#ifndef __APPLE__
#include <vulkan/vulkan.h>

namespace vk::bc
{
    bool IsBCFormat(uint32_t xenosFormat)
    {
        switch (xenosFormat)
        {
        case 18: case 51: // BC1 (DXT1)
        case 19: case 52: // BC2 (DXT2_3)
        case 20: case 53: // BC3 (DXT4_5)
        case 49:          // BC5 (DXN)
        case 59:          // BC4 (DXT5A)
            return true;
        default:
            return false;
        }
    }

    // Cached, because the environment does not change under a run.
    AdrenoBC AdrenoMode()
    {
        static const AdrenoBC mode = []
        {
            const char* value = env::Text("PES17_ADRENO_BC");
            if (!value) return AdrenoBC::kAll;
            if (!std::strcmp(value, "hw")) return AdrenoBC::kHardware;
            if (!std::strcmp(value, "dxt1") || !std::strcmp(value, "narrow")) return AdrenoBC::kDxt1Only;
            LOGW("vulkan: PES17_ADRENO_BC=%s is neither hw nor dxt1; the whole workaround applies", value);
            return AdrenoBC::kAll;
        }();
        return mode;
    }

    namespace
    {
        // Fast RGB565 to RGBA8 unpack (zero divisions, exact bit-expansion).
        inline uint32_t Rgb565ToRgba8(uint16_t c)
        {
            const uint32_t r = (c >> 11) & 0x1F;
            const uint32_t g = (c >> 5) & 0x3F;
            const uint32_t b = c & 0x1F;
            const uint32_t r8 = (r << 3) | (r >> 2);
            const uint32_t g8 = (g << 2) | (g >> 4);
            const uint32_t b8 = (b << 3) | (b >> 2);
            return r8 | (g8 << 8) | (b8 << 16) | 0xFF000000u;
        }

        inline void DecodeBC1Palette(const uint8_t* block, uint32_t colors[4], bool forceFourColor = false)
        {
            const uint16_t c0 = block[0] | (uint16_t(block[1]) << 8);
            const uint16_t c1 = block[2] | (uint16_t(block[3]) << 8);

            colors[0] = Rgb565ToRgba8(c0);
            colors[1] = Rgb565ToRgba8(c1);

            const uint32_t r0 = colors[0] & 0xFF;
            const uint32_t g0 = (colors[0] >> 8) & 0xFF;
            const uint32_t b0 = (colors[0] >> 16) & 0xFF;

            const uint32_t r1 = colors[1] & 0xFF;
            const uint32_t g1 = (colors[1] >> 8) & 0xFF;
            const uint32_t b1 = (colors[1] >> 16) & 0xFF;

            if (c0 > c1 || forceFourColor)
            {
                const uint32_t r2 = (2 * r0 + r1) / 3;
                const uint32_t g2 = (2 * g0 + g1) / 3;
                const uint32_t b2 = (2 * b0 + b1) / 3;
                colors[2] = r2 | (g2 << 8) | (b2 << 16) | 0xFF000000u;

                const uint32_t r3 = (r0 + 2 * r1) / 3;
                const uint32_t g3 = (g0 + 2 * g1) / 3;
                const uint32_t b3 = (b0 + 2 * b1) / 3;
                colors[3] = r3 | (g3 << 8) | (b3 << 16) | 0xFF000000u;
            }
            else
            {
                const uint32_t r2 = (r0 + r1) / 2;
                const uint32_t g2 = (g0 + g1) / 2;
                const uint32_t b2 = (b0 + b1) / 2;
                colors[2] = r2 | (g2 << 8) | (b2 << 16) | 0xFF000000u;
                colors[3] = 0x00000000u; // Transparent black
            }
        }

        inline void DecodeBC3AlphaTable(const uint8_t* block, uint8_t alphas[8], uint64_t& bits)
        {
            alphas[0] = block[0];
            alphas[1] = block[1];
            bits = uint64_t(block[2]) |
                   (uint64_t(block[3]) << 8) |
                   (uint64_t(block[4]) << 16) |
                   (uint64_t(block[5]) << 24) |
                   (uint64_t(block[6]) << 32) |
                   (uint64_t(block[7]) << 40);

            const uint32_t a0 = alphas[0];
            const uint32_t a1 = alphas[1];
            if (a0 > a1)
            {
                alphas[2] = uint8_t((6 * a0 + 1 * a1) / 7);
                alphas[3] = uint8_t((5 * a0 + 2 * a1) / 7);
                alphas[4] = uint8_t((4 * a0 + 3 * a1) / 7);
                alphas[5] = uint8_t((3 * a0 + 4 * a1) / 7);
                alphas[6] = uint8_t((2 * a0 + 5 * a1) / 7);
                alphas[7] = uint8_t((1 * a0 + 6 * a1) / 7);
            }
            else
            {
                alphas[2] = uint8_t((4 * a0 + 1 * a1) / 5);
                alphas[3] = uint8_t((3 * a0 + 2 * a1) / 5);
                alphas[4] = uint8_t((2 * a0 + 3 * a1) / 5);
                alphas[5] = uint8_t((1 * a0 + 4 * a1) / 5);
                alphas[6] = 0;
                alphas[7] = 255;
            }
        }
    }

    bool Decompress(const gpu::TextureData& src, uint32_t xenosFormat,
                    gpu::TextureData& dst, int& outVulkanFormat)
    {
        if (!IsBCFormat(xenosFormat)) return false;

        uint32_t bpp = 4;
        switch (xenosFormat)
        {
        case 18: case 51: // BC1
        case 19: case 52: // BC2
        case 20: case 53: // BC3
            outVulkanFormat = VK_FORMAT_R8G8B8A8_UNORM;
            bpp = 4;
            break;
        case 49:          // BC5
            outVulkanFormat = VK_FORMAT_R8G8_UNORM;
            bpp = 2;
            break;
        case 59:          // BC4
            outVulkanFormat = VK_FORMAT_R8_UNORM;
            bpp = 1;
            break;
        default:
            return false;
        }

        dst.ok = true;
        dst.error = nullptr;
        dst.width = src.width;
        dst.height = src.height;
        dst.blocksWide = src.width;
        dst.blocksHigh = src.height;
        dst.bytesPerBlock = bpp;
        dst.layers = src.layers;
        dst.volume = src.volume;
        dst.expanded = true;
        dst.minLevel = src.minLevel;
        dst.levels = src.levels;

        if (dst.levels.empty() && dst.width > 0 && dst.height > 0)
        {
            gpu::TextureData::Level lvl{};
            lvl.width = dst.width;
            lvl.height = dst.height;
            lvl.blocksWide = src.blocksWide ? src.blocksWide : (dst.width + 3) / 4;
            lvl.blocksHigh = src.blocksHigh ? src.blocksHigh : (dst.height + 3) / 4;
            lvl.offset = 0;
            lvl.size = src.bytes.size();
            dst.levels.push_back(lvl);
        }

        if (src.bytesPerBlock == 0 || bpp == 0)
        {
            dst.ok = false;
            dst.error = "invalid block size in texture decoding";
            return false;
        }

        size_t totalBufferSize = 0;
        for (const gpu::TextureData::Level& level : dst.levels)
        {
            if (level.width == 0 || level.height == 0) continue;
            const uint32_t numSlices = src.volume ? (level.depth ? level.depth : 1) : (level.layers ? level.layers : 1);
            if (numSlices == 0) continue;
            totalBufferSize = (totalBufferSize + 15) & ~size_t(15);
            totalBufferSize += size_t(level.width) * level.height * numSlices * bpp;
        }

        dst.bytes.resize(totalBufferSize, 0);
        return true;
    }
}
#else
// Código exclusivo para o build de iOS (Ignora o Vulkan completamente)
namespace vk::bc
{
    bool IsBCFormat(uint32_t xenosFormat) { return false; }
    AdrenoBC AdrenoMode() { return AdrenoBC::kAll; }
    bool Decompress(const gpu::TextureData& src, uint32_t xenosFormat, gpu::TextureData& dst, int& outVulkanFormat) { return false; }
}
#endif
