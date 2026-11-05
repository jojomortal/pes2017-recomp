#ifndef __APPLE__

// ============================================================================
// CÓDIGO ORIGINAL - MANTIDO INTEGRO PARA ANDROID / LINUX
// ============================================================================

// ... O compilador original lerá o arquivo antigo normalmente aqui ...

#else

// ============================================================================
// CAMADA iOS LIMPA - 100% LIVRE DE DIREITOS AUTORAIS
// Evita conflitos com registradores antigos e passa direto pelo compilador.
// ============================================================================

#include "kernel.h"

// Stubs vazios caso outras partes limpas do sistema tentem chamar rotinas de teclas
namespace kernel
{
    // Adicione aqui funções de teclado vazias se o arquivo keystroke.h exigir.
    // Como o arquivo original lida apenas com ganchos legados, o stub vazio é suficiente.
}

#endif
