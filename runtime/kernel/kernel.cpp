#ifndef __APPLE__

// ============================================================================
// O compilador original (Android/Linux) continuará lendo todo o código antigo
// que já existia no arquivo normalmente por causa desta diretiva.
// ============================================================================

#include "kernel.h"
#include "title.h"
#include "../guest.h"
#include "../diagnostics.h"
#include "../log.h"
#include "../crash.h"
#include "../gpu/memory_watch.h"
#include <map>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <cstring>

// ... Todo o resto do seu código original que já estava aí continua aqui ...

#else

// ============================================================================
// CAMADA iOS LIMPA - 100% LIVRE DE DIREITOS AUTORAIS
// Se for compilar no iOS, o sistema lê apenas estas definições neutras.
// ============================================================================

#include "kernel.h"
#include <cstdint>
#include <string>

namespace kernel
{
    void Init(const std::filesystem::path& gameRoot) {}
    void Shutdown() {}
    void Run() {}
    
    // Stubs vazios e neutros para garantir que o launcher do iOS compile sem dependências antigas
    uint32_t AllocateGuestMemory(uint32_t size, uint32_t align) { return 0; }
    void FreeGuestMemory(uint32_t address) {}
    bool IsGuestAddressValid(uint32_t address) { return false; }
}

#endif
