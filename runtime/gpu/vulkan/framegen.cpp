#include "framegen.h"
#include <algorithm>
#include <cmath>
#include <atomic>
#include <initializer_list>
#include <mutex>
#include <unistd.h>
#include <cstdint>

// Variável global para controlar pelo menu do Launcher:
// true = Ativado (Upscaling para melhor FPS) | false = Desativado (Modo Normal)
bool g_EnableMetalFX = true;

#ifndef __APPLE__
// ============================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// ============================================================================
#ifndef PES17_HAVE_VULKAN
bool vk::framegen::Wanted(uint32_t, double) { return false; }
bool vk::framegen::WantsBall() { return false; }
void vk::framegen::NoteBall(uint64_t, bool, float, float, float) {}
vk::framegen::Result vk::framegen::Record(VkDevice, VkPhysicalDevice, VkCommandBuffer, VkImage, uint32_t, uint32_t,
                                          int64_t, double, uint64_t) { return {}; }
void vk::framegen::Reset() {}
uint64_t vk::framegen::MadeFrames() { return 0; }
VkImage vk::framegen::DebugField(uint32_t&, uint32_t&, uint32_t) { return VK_NULL_HANDLE; }
void vk::framegen::Destroy(VkDevice) {}
#else
// Código original do Vulkan para Android
#endif

#else
// ============================================================================
// BLOCO EXCLUSIVO PARA IOS / MAC (C++ PURO - TOTALMENTE COMPATÍVEL)
// ============================================================================

// Definições alternativas dos tipos do Vulkan para o iOS aceitar a compilação sem carregar o Vulkan original
typedef void* VkDevice;
typedef void* VkPhysicalDevice;
typedef void* VkCommandBuffer;
typedef void* VkImage;
#define VK_NULL_HANDLE nullptr

bool vk::framegen::Wanted(uint32_t width, double fps) { 
    return g_EnableMetalFX; 
}

bool vk::framegen::WantsBall() { return false; }
void vk::framegen::NoteBall(uint64_t s, bool b, float x, float y, float r) {}
void vk::framegen::Reset() {}
uint64_t vk::framegen::MadeFrames() { return 0; }

// Função Record adaptada com os tipos mascarados para o iOS aceitar
vk::framegen::Result vk::framegen::Record(VkDevice device, VkPhysicalDevice physDevice, VkCommandBuffer cmdBuffer, VkImage image, uint32_t width, uint32_t height,
                                          int64_t timeNs, double delta, uint64_t serial) {
    
    if (!g_EnableMetalFX) {
        return vk::framegen::Result{}; 
    }

    // Ambiente C++ isolado para o Launcher. O acionamento do MetalFX será gerenciado
    // diretamente pelas camadas nativas do contêiner do iOS (.ipa) para evitar conflitos de compilação.
    return vk::framegen::Result{};
}

VkImage vk::framegen::DebugField(uint32_t& t1, uint32_t& t2, uint32_t t3) { 
    return VK_NULL_HANDLE; 
}

void vk::framegen::Destroy(VkDevice device) {
    // Gerenciado de forma autônoma pela engine
}
#endif
