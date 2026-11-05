#ifndef __APPLE__
// What a draw reads besides its textures -- constants, vertex data, indices --
// copied out of guest memory and the register file into the frame arena.
#include "renderer_state.h"
#include "../memory_watch.h"
#include "../../guest.h"
#include "../../kernel/physical.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdio>
#include <vector>

#ifndef __APPLE__
// =========================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// =========================================================================

#ifdef PES17_ANDROID
#include "../../android/android.h"
#endif

#ifdef PES17_HAVE_VULKAN

namespace vk::renderer::detail
{
    // The vertex shadow (gpu::watch::Serve): unchanged vertex pages read from a
    // GPU-visible copy instead of copied for every draw. Off: with it on, the
    // match's players drew only their shadows and the kit compositions came out
    // wrong -- their vertex data is rewritten in ways the page watch misses.
    // files/vertex_shadow turns it on, for testing.
    const bool kServeVertexShadow = [] {
#ifdef PES17_ANDROID
        if (FILE* f = std::fopen((android::GetPaths().external + "/vertex_shadow").c_str(), "r"))
        { std::fclose(f); return true; }
#endif
        return false;
    }();
    // A draw's constants, vertex data and indices are copied here and addressed by
    // offset, and it is reset when the frame is submitted.
    uint32_t Allocate(uint32_t bytes, ArenaKind kind)
    {
        const uint32_t aligned = (g.arenaUsed + g.uniformAlignment - 1) & ~(g.uniformAlignment - 1);
        if (aligned + bytes > g.arenaSlotBytes)
        {
            g.arenaOverflows++;
            return UINT32_MAX;
        }
        g.arenaUsed = aligned + bytes;
        g.arenaWritten[kind] += bytes;
        // Offsets are into the whole buffer -- they become descriptor offsets and
        // storage-buffer indices -- so the slot's base is carried in the answer.
        return g.arenaBase + aligned;
    }

    uint32_t CopyIntoArena(const void* bytes, uint32_t size, ArenaKind kind)
    {
        const uint32_t at = Allocate(size, kind);
        if (at == UINT32_MAX) return at;
        std::memcpy(g.arenaMapped + at, bytes, size);
        return at;
    }

    // The file holds 512 vec4 and a stage sees 256 from wherever its base says --
    // for a pixel shader usually 256, so uploading the low half twice would hand
    // it the vertex shader's constants.
    // The uniform block descriptor range is the full 256 vec4 (4 KB). Skinned
    // models, light grid spherical harmonic probes, and mobile Mali GPU vectorised
    // uniform loads require the full 256-vec4 window to be fully populated so
    // lighting and bone constants are never truncated or read as uninitialised.
    uint32_t WriteConstantWindow(const gpu::RegisterFile& r, uint32_t windowRegister,
                                 uint32_t slot, uint32_t /*needed*/)
    {
        const gpu::ConstantWindow window{ r[windowRegister] };
        const uint32_t base = std::min(window.Base(), 256u);
        const uint32_t first = gpu::kConstantBaseAlu + base * 4;
        constexpr uint32_t kWindowCount = 256;
        constexpr uint32_t bytes = kWindowCount * 16;

        State::Cached& cache = g.constantWindows[slot];
        if (cache.valid && cache.base == base &&
            cache.stamp == r.StampOf(first, kWindowCount * 4))
        {
            g.arenaReused[kArenaConstants] += bytes;
            return cache.at;
        }

        const uint32_t at = Allocate(bytes, kArenaConstants);
        if (at == UINT32_MAX) return at;
        std::memcpy(g.arenaMapped + at, &r.values[first], bytes);
        cache = { r.StampOf(first, kWindowCount * 4), base, at, true, kWindowCount };
        return at;
    }

    // What a program needs copied: one past the highest constant it names, or
    // the whole window when it indexes them with a run-time value.
    uint32_t ConstantsNeeded(const shader::Translation& program)
    {
        if (program.addressedConstants) return 256;
        return std::min(program.constantsRead, 256u);
    }

    // Xenos takes three corners of a rectangle and works out the fourth as
    // v0 + v2 - v1. The data is still in the guest's byte order here, and a field
    // that is the same at all three corners -- a packed colour, most often -- is
    // carried across rather than run through arithmetic that would not mean
    // anything for it.
    void BuildFourthCorner(uint32_t* vertices, uint32_t stride, uint32_t rect,
                           uint32_t at)
    {
        const uint32_t* v0 = vertices + (rect * 3 + 0) * stride;
        const uint32_t* v1 = vertices + (rect * 3 + 1) * stride;
        const uint32_t* v2 = vertices + (rect * 3 + 2) * stride;
        uint32_t* out = vertices + at * stride;
        for (uint32_t d = 0; d < stride; d++)
        {
            const uint32_t a = __builtin_bswap32(v0[d]);
            const uint32_t b = __builtin_bswap32(v1[d]);
            const uint32_t c = __builtin_bswap32(v2[d]);
            const uint32_t value =
                (a == b && b == c)
                    ? a
                    : std::bit_cast<uint32_t>(std::bit_cast<float>(a) +
                                              std::bit_cast<float>(c) -
                                              std::bit_cast<float>(b));
            out[d] = __builtin_bswap32(value);
        }
    }

    uint64_t VertexRange(uint32_t lowest, uint32_t highest)
    {
        return (uint64_t(lowest) << 32) | highest;
    }

    ConstantOffsets WriteConstants(const gpu::RegisterFile& r,
                                   const shader::Translation& program,
                                   const shader::Translation& pixelProgram,
                                   uint32_t lowestVertex, uint32_t highestVertex,
                                   uint32_t rectangles, float modelScale)
    {
        const uint32_t (&subs)[3] = program.vertexFetchSubs;
        ConstantOffsets out{ 0, 0, 0, 0, 0, false };

        out.vertexFloats = WriteConstantWindow(r, gpu::SQ_VS_CONST, 0,
                                               ConstantsNeeded(program));
        if (out.vertexFloats == UINT32_MAX) return out;
        if (modelScale != 1.0f)
        {
            // A model's world matrix is c0..c2 (three rows, translation in w). Its
            // rotation part scaled grows the model about its own centre. The shared
            // window is cached for other draws, so the scaled one is a copy.
            constexpr uint32_t bytes = bindings::kBlockBytes[bindings::kVertexFloats];
            const uint32_t at = Allocate(bytes, kArenaConstants);
            if (at == UINT32_MAX) { out.vertexFloats = UINT32_MAX; return out; }
            std::memcpy(g.arenaMapped + at, g.arenaMapped + out.vertexFloats, bytes);
            float* rows = reinterpret_cast<float*>(g.arenaMapped + at);
            for (uint32_t row = 0; row < 3; row++)
                for (uint32_t k = 0; k < 3; k++) rows[row * 4 + k] *= modelScale;
            out.vertexFloats = at;
        }
        out.pixelFloats = WriteConstantWindow(r, gpu::SQ_PS_CONST, 1,
                                              ConstantsNeeded(pixelProgram));
        if (out.pixelFloats == UINT32_MAX) return out;
        if (g.drawRecord)
            NoteConstantWindows(r, ConstantsNeeded(program), ConstantsNeeded(pixelProgram));

        // The assembled block depends on the fetch registers and on which groups
        // the shader reads, and the vertex copies it points at stay where they are
        // for the rest of the frame -- so an unchanged pair is the same block.
        const uint64_t fetchStamp = r.StampOf(gpu::kConstantBaseFetch, 32 * 6);
        // A rectangle list gets a copy of its own, with the corners it does not
        // carry written into it -- so it can never share one with another draw.
        const bool fetchCached = !rectangles &&
                                 g.fetchBlock.valid && g.fetchBlock.stamp == fetchStamp &&
                                 g.fetchBlock.base == VertexRange(lowestVertex, highestVertex) &&
                                 std::equal(subs, subs + 3, g.fetchSubs);
        return out;
    }
}

#endif // PES17_HAVE_VULKAN
#endif // __APPLE__
