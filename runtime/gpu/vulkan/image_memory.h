#pragma once
#include <cstdint>

// Se for compilando para iOS, ignora completamente o Vulkan e cria assinaturas vazias seguras
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED)

namespace vk::imagememory 
{
    struct Range {
        void* memory = nullptr;
        uint64_t offset = 0, size = 0;
        uint32_t block = 0xFFFFFFFF;
        explicit operator bool() const { return memory != nullptr; }
    };

    inline Range Allocate(void* device, const void* needs, uint32_t type) { return Range(); }
    inline void Free(void* device, Range& range) {}
    inline void Shutdown(void* device) {}

    struct Totals { uint64_t blocks = 0, blockBytes = 0, usedBytes = 0, alone = 0, aloneBytes = 0; };
    inline Totals Held() { return Totals(); }
}

#else
// Caso contrário (Android/Windows), mantém o Vulkan original intacto
#include <vulkan/vulkan.h>

namespace vk::imagememory
{
#if defined(PES17_ANDROID) || defined(_ANDROID_)
    constexpr VkDeviceSize kBlockBytes = 16ull << 20;
#else
    constexpr VkDeviceSize kBlockBytes = 64ull << 20;
#endif

    struct Range
    {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize offset = 0, size = 0;
        uint32_t block = 0xFFFFFFFF; // which block; UINT32_MAX for memory of its own
        explicit operator bool() const { return memory != VK_NULL_HANDLE; }
    };

    Range Allocate(VkDevice device, const VkMemoryRequirements& needs, uint32_t type);
    void Free(VkDevice device, Range& range);
    void Shutdown(VkDevice device);

    struct Totals { uint64_t blocks = 0, blockBytes = 0, usedBytes = 0, alone = 0, aloneBytes = 0; };
    Totals Held();
}
#endif
