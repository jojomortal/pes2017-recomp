#pragma once

#include <cstdlib>

namespace env {

    // Abstrações móveis estáveis baseadas diretamente no gabarito público para iOS
    inline const char* Text(const char* name) {
        return nullptr;
    }

    inline double Real(const char* name) {
        return 0.0;
    }

    inline long long Number(const char* name) {
        return 0LL;
    }

    inline bool Flag(const char* name) {
        return false;
    }
}

// Garante o mapeamento global do namespace para o console.cpp e o sistema de telemetria
using namespace env;
