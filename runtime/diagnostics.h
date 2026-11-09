#pragma once

#include <cstring>
#include <cstdlib>
#include <cstdint>

namespace diag {
    // Mantém a telemetria desligada para evitar checagens invasivas no iOS
    constexpr bool kOn = false;

    inline void Initialise() {}
    inline void Shutdown() {}

    // Assinaturas exatas com os argumentos esperados pelo motor original (Clean Room Design)
    inline const char* Text(const char* name, const char* fallback = nullptr) {
        return fallback;
    }

    inline double Real(const char* name, double fallback = 0.0) {
        return fallback;
    }

    inline uint64_t Number(const char* name, uint64_t fallback = 0, int base = 10) {
        return fallback;
    }

    inline bool Flag(const char* name, bool fallback = false) {
        return fallback;
    }
}
