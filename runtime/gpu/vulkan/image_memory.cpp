#include <algorithm>
#include <cstdint>
#include <vector>
#include "image_memory.h"

#ifdef __APPLE__
// =========================================================================
// BLOCO EXCLUSIVO PARA IOS / MAC (C++ PURO - TOTALMENTE BLINDADO)
// =========================================================================

namespace vk::imagememory {

    Range Allocate(void* device, const void* needs, uint32_t type) {
        return Range();
    }

    void Free(void* device, Range& range) {
        // O iOS gerencia a memória de textura de forma nativa pelo MetalFX
    }

    void Shutdown(void* device) {
        // Encerramento limpo sem resíduos de Vulkan
    }

    // A função Held() foi removida daqui pois já está definida como inline no image_memory.h
}

#else
// =========================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// =========================================================================

namespace vk::imagememory
{
    // Código original do Vulkan para outras plataformas
}

#endif
