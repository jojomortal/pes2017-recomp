#pragma once
#include <cmath>
#include "../../log.h"
// The renderer's state and the functions its parts share. Internal to the
// renderer_*.cpp files; everyone else goes through renderer.h.
//
//   renderer.cpp              the state, start-up and shutdown
//   renderer_draw.cpp         a draw: targets, state, bindings, the command
//   renderer_targets.cpp      EDRAM surfaces as images, passes, framebuffers, clears
//   renderer_resolve.cpp      resolves: to a texture, or the frame to the window
//   renderer_frame.cpp        command buffers, submission, frame slots, queries
//   renderer_arena.cpp        what a draw reads copied into the frame arena
//   renderer_pipelines.cpp    shaders, pipelines and the shader cache
//   renderer_libraries.cpp    shaders compiled at load as pipeline libraries, and linking
//   renderer_diagnostics.cpp  the switches that look at a frame, and the report
#include "renderer.h"
#include "../../diagnostics.h"
#include "bindings.h"
#include "dynamic_scale.h"
#include "texture_cache.h"
#include "pipeline.h"
#include "recorder.h"
#include "shader_translator.h"
#include "util.h"
#include "../registers.h"
#include "../texture.h"
#include "../../env.h"
#include "../../stutters.h"
#include "../../words_hash.h"

#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef __APPLE__
  #include <vulkan/vulkan.h>
#else
  // =================================================================
  // STUBS COMPLETO DE COMPATIBILIDADE VULKAN PARA O ECOSSISTEMA APPLE
  // =================================================================
  typedef void* VkImage;
  typedef void* VkDeviceMemory;
  typedef void* VkImageView;
  typedef void* VkShaderModule;
  typedef void* VkPipeline;
  typedef void* VkRenderPass;

  // Enums e Constantes fictícias consumidas pelo header
  enum VkFormat {
      VK_FORMAT_UNDEFINED = 0
  };

  enum VkSampleCountFlagBits {
      VK_SAMPLE_COUNT_1_BIT = 1
  };

  #define VK_NULL_HANDLE nullptr
#endif

namespace vk::renderer::detail
{
    // Per frame, reset at the resolve. A frame that runs out goes black from
    // the point it ran out, which reads as a shading bug.
    constexpr uint32_t kArenaWanted = 512u << 20;
    constexpr uint32_t kArenaLeast  = 32u << 20;

    // Split, because the total only says a frame ran out of room, not on what.
    enum ArenaKind { kArenaConstants, kArenaVertices, kArenaIndices, kArenaKinds };
    constexpr const char* kArenaNames[kArenaKinds] = { "constants", "vertex data", "indices" };
    // Four rather than two: the window shows every frame, a blank each, and
    // each image is kept intact until the window has copied it out. Four let
    // the renderer run up to three frames ahead of the screen before it waits.
    constexpr uint32_t kPresentImages = 4;
    // How many submissions the GPU may be holding while the CPU records the
    // next; a frame is about six. Each needs its own share of the arena, and a
    // heavy MP submission copies 87 MB into it: at six slots the shares
    // overflowed and draws were dropped, at three they never do.
    constexpr uint32_t kFrameSlots = 3;
    // Command buffers made for a slot at the start, and again each time a
    // submission needs more segments than it has: a frame has about twenty.
    constexpr uint32_t kSegmentBuffers = 32;
    static_assert(kFrameSlots <= vk::textures::kUploadSlots, "a slot's uploads need a slot too");
    // The title's own occlusion queries. One of the title's brackets takes as
    // many of these as it covers render passes, because a Vulkan query has to
    // begin and end inside one of those, and their counts are summed. The pool
    // is a ring, so brackets whose counts are not in yet keep theirs.
    constexpr uint32_t kMaxGuestQueries = 64;
    constexpr uint32_t kGuestQueryRing = 4 * kMaxGuestQueries;

    // A tile of EDRAM is one surface however the title declares it, so the key
    // holds the host format: 8_8_8_8 and its gamma twin are one image, which is
    // what a title relies on when it resolves a pass it drew under the other name.
    struct TargetKey
    {
        uint32_t baseTile = 0, pitch = 0, format = 0, samples = 0;
        bool depth = false;
        auto operator<=>(const TargetKey&) const = default;
    };

    // Everything about a pipeline: the two shaders, the formats of the targets
    // they draw into (dynamic rendering: there is no render pass object), and
    // the fixed-function register bits they are drawn with.
    struct PipelineKey
    {
        uint64_t vertexShader = 0, pixelShader = 0;
        uint32_t colourFormat = 0, depthFormat = 0;
        uint32_t topology = 0;
        uint32_t blendControl = 0;
        uint32_t colourMask = 0;
        uint32_t samples = 1;   // the render pass's, so the rasteriser matches it
        uint32_t depthControl = 0; // for legacy mode where depth/stencil is static
        uint32_t modeCntl = 0;     // for legacy mode where cull/frontface is static
        auto operator<=>(const PipelineKey&) const = default;
    };

    struct Target
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t guestFormat = 0;   // for the report
        uint32_t width = 0, height = 0;
        // The scene render scale (State::sceneScale) for a full-frame surface,
        // 1 for the rest: the image is HostOf(width, scale) wide.
        float scale = 1.0f;
        bool depth = false;
        // The console's 2x and 4x surfaces are drawn with that many samples
        // here. A multisampled image can be neither blitted, copied nor
        // sampled, so every read of one -- a resolve, the present, a dump --
        // goes through a single-sampled twin the same size, made on first use
        // and filled by the read's own resolve.
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkImage singleImage = VK_NULL_HANDLE;
        VkDeviceMemory singleMemory = VK_NULL_HANDLE;
        VkImageView singleView = VK_NULL_HANDLE;
        bool singleReady = false;   // holds a resolve of some rectangle
        // A surface nothing presents looks exactly like a surface nothing draws into.
        uint64_t draws = 0, presented = 0;
        // Colour draws this frame; a multisampled target with any is the world pass.
        uint64_t colourDrawsThisFrame = 0;
        // How many presented frames drew colour into this target at all. A
        // surface the title fills every frame and one it fills every third
        // are the same total and a different picture.
        uint64_t framesWithColour = 0;
        // The draw count at which an EDRAM clear last emptied this, so the
        // second rectangle of a clear pair does not clear it again.
        uint64_t clearedAtDraw = UINT64_MAX;
        // The frame that clear happened in, for the pass trace.
        uint64_t depthClearedIn = UINT64_MAX;
    };

    struct Shader
    {
        shader::Translation translation;
        VkShaderModule module = VK_NULL_HANDLE;
        // The shader compiled on its own, as a pipeline library for its stage
        // (renderer_libraries.cpp); null on a device without them.
        VkPipeline library = VK_NULL_HANDLE;
        shader::Type type = shader::Type::Vertex;
        std::vector<uint32_t> code;     // kept so the shader cache can be written
    };

    // What a pipeline's output stage is built from: the blend, the write mask,
    // and the targets' formats and sample count.
    struct OutputKey
    {
        uint32_t blendControl = 0, colourMask = 0;
        uint32_t colourFormat = 0, depthFormat = 0, samples = 1;
        auto operator<=>(const OutputKey&) const = default;
    };

    struct RenderPassKey
    {
        VkFormat colourFormat = VK_FORMAT_UNDEFINED;
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        auto operator<=>(const RenderPassKey&) const = default;
    };

    struct FramebufferKey
    {
        VkRenderPass pass = VK_NULL_HANDLE;
        VkImageView colourView = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        auto operator<=>(const FramebufferKey&) const = default;
    };

    // A pipeline a previous run needed, with the formats its render pass was
    // built from, so it can be rebuilt before the title asks for it.
    struct Recorded
    {
        uint64_t vertexShader = 0, pixelShader = 0;
        uint32_t colourFormat = 0, depthFormat = 0;
    };
}
