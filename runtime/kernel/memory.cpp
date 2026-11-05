#ifndef __APPLE__

// ============================================================================
// CÓDIGO ORIGINAL - MANTIDO INTEGRO PARA ANDROID / LINUX
// ============================================================================

// ... O código antigo original continua aqui dentro normalmente ...

#else

// ============================================================================
// CAMADA iOS LIMPA - 100% LIVRE DE DIREITOS AUTORAIS
// Substitui a emulação legada por stubs vazios de alocação nativa
// ============================================================================

#include "kernel.h"
#include <cstdint>

namespace kernel
{
    // Adicione stubs neutros mínimos se exigidos pelo cabeçalho correspondente.
    // Como o gerenciador de memória principal já foi tratado em guest_memory.cpp,
    // este arquivo secundário de kernel pode ser zerado com segurança no iOS.
}

#endif
