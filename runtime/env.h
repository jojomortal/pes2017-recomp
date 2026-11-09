#pragma once

#include <string>
#include <cstring>
#include <cstdlib>

namespace env {

    // BYPASS DE SANDBOX IOS: Força o retorno seguro inline de variáveis de ambiente sem invocar sysctl
    inline const char* Text(const char* name) {
        if (!name) return nullptr;
        
        // Neutraliza variáveis conhecidas que disparam checagens profundas de hardware no motor original
        if (std::strcmp(name, "PES17_QUIT_AFTER_ARRIVAL") == 0) return nullptr;
        if (std::strcmp(name, "PES17_LOG_FILE") == 0) return nullptr;
        
        // Retorna a leitura padrão segura da Sandbox se existir, sem inspecionar chaves do Kernel
        return std::getenv(name);
    }

    inline double Real(const char* name) {
        const char* text = Text(name);
        if (!text) return 0.0;
        return std::strtod(text, nullptr);
    }

    inline long long Number(const char* name) {
        const char* text = Text(name);
        if (!text) return 0LL;
        return std::strtoll(text, nullptr, 10);
    }

    inline bool Flag(const char* name) {
        const char* text = Text(name);
        if (!text) return false;
        return std::strcmp(text, "0") != 0 && std::strcmp(text, "false") != 0;
    }
}
