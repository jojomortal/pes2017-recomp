#pragma once
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
// =========================================================================
// MOCK BLINDADO PARA IOS (SEM VULKAN NATIVO)
// =========================================================================

// Substitutos genéricos para compilar no iOS sem o SDK do Vulkan
typedef void* VkCommandBuffer;

namespace vk::record
{
    inline void Start() {}
    inline void Stop() {}
    inline bool Threaded() { return false; }
    inline void Into(VkCommandBuffer command) {}
    inline uint64_t Position() { return 0; }
    inline bool Done(uint64_t position) { return true; }
    inline void WaitFor(uint64_t position) {}
    inline void Drain() {}
    inline void Report() {}

    namespace detail
    {
        struct alignas(32) Header
        {
            void (*run)(void* self, VkCommandBuffer command);
            VkCommandBuffer command;
            uint32_t bytes;
        };
        inline bool threaded = false;
        inline VkCommandBuffer into = nullptr;
        inline void* Reserve(uint32_t bytes) { return nullptr; }
        inline void Publish() {}
    }

    template <class Call> void Command(Call&& call)
    {
        // No iOS roda diretamente de forma síncrona/vazia se for acionado
        if (!detail::threaded) { return; }
    }

    template <class Call> void Host(Call&& call)
    {
        // Versão inline segura para evitar erros de compilação por falta do tipo original
    }
}

#else
// =========================================================================
// CÓDIGO ORIGINAL PARA OUTRAS PLATAFORMAS (ANDROID / PC)
// =========================================================================
#include <vulkan/vulkan.h>

namespace vk::record
{
    void Start();
    void Stop();
    bool Threaded();

    void Into(VkCommandBuffer command);

    uint64_t Position();
    bool Done(uint64_t position);
    void WaitFor(uint64_t position);
    void Drain();

    void Report();

    namespace detail
    {
        struct alignas(32) Header
        {
            void (*run)(void* self, VkCommandBuffer command);
            VkCommandBuffer command;
            uint32_t bytes;
        };
        static_assert(sizeof(Header) == 32);
        extern bool threaded;
        extern VkCommandBuffer into;
        void* Reserve(uint32_t bytes);
        void Publish();
    }

    template <class Call> void Command(Call&& call)
    {
        using T = std::decay_t<Call>;
        if (!detail::threaded) { call(detail::into); return; }
        static_assert(alignof(T) <= alignof(detail::Header));
        constexpr uint32_t bytes =
            uint32_t((sizeof(detail::Header) + sizeof(T) + sizeof(detail::Header) - 1) &
                     ~(sizeof(detail::Header) - 1));
        void* at = detail::Reserve(bytes);
        auto* header = new (at) detail::Header{
            [](void* self, VkCommandBuffer command) {
                T* made = static_cast<T*>(self);
                (*made)(command);
                made->~T();
            },
            detail::into, bytes };
        new (header + 1) T(std::forward<Call>(call));
        detail::Publish();
    }

    template <class Call> void Host(Call&& call)
    {
        Command([call = std::forward<Call>(call)](VkCommandBuffer) mutable { call(); });
    }
}
#endif
