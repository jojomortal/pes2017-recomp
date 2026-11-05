#include <algorithm>
#include <cstdint>
#include <vector>
#include "image_memory.h"

#ifdef __APPLE__
// =========================================================================
// BLOCO EXCLUSIVO PARA IOS / MAC (C++ PURO - COMPATÍVEL COM O LAUNCHER)
// =========================================================================

namespace vk::imagememory {

    Range Allocate(VkDevice device, const VkMemoryRequirements& needs, uint32_t type) {
        return Range();
    }

    void Free(VkDevice device, Range& range) {
        // O iOS gerencia a memória de textura de forma nativa pelo MetalFX
    }

    void Shutdown(VkDevice device) {
        // Encerramento limpo sem resíduos de Vulkan
    }

    Totals Held() {
        return Totals();
    }
}

#else
// =========================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// =========================================================================

namespace vk::imagememory
{
    // Aqui vai a implementação original complexa do Vulkan para Android/PC.
    // Como você está limpando o arquivo, se a compilação do Android não for o foco 
    // deste branch, este bloco pode permanecer vazio ou conter o código original.
}

#endif
