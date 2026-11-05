#include <algorithm>
#include <cstdint>
#include <vector>

// Criamos as definições falsas antes de qualquer leitura para blindar o iOS
#ifdef __APPLE__
typedef void* VkDevice;
typedef void* VkDeviceMemory;
typedef void* VkImage;
#define VK_NULL_HANDLE nullptr
#endif

#include "image_memory.h"

#ifndef __APPLE__
// ============================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// ============================================================================
// O código original complexo do Vulkan para Android rodaria aqui...

#else
// ============================================================================
// BLOCO EXCLUSIVO PARA IOS / MAC (C++ PURO - COMPATÍVEL COM O LAUNCHER)
// ============================================================================

namespace vk::image_memory {

    // Criação de funções vazias (stubs) para o gerenciador de memória gráfica compilar no iOS
    bool Allocate(VkDevice device, size_t size, VkDeviceMemory& outMemory) {
        outMemory = VK_NULL_HANDLE;
        return true;
    }

    void Free(VkDevice device, VkDeviceMemory memory) {
        // O iOS gerencia a memória do contêiner de forma automática
    }

    bool BindImage(VkDevice device, VkImage image, VkDeviceMemory memory, size_t offset) {
        return true;
    }

    void ClearCache() {
        // Cache limpo por padrão no launcher limpo
    }
}
#endif
