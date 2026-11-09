#pragma once

#include <cstdlib>

namespace env {

    // Retorna valores seguros mockados para o ambiente mobile (Clean Room Design)
    inline const char* Text(const char* name, const char* fallback = nullptr) {
        return fallback;
    }

    inline double Real(const char* name, double fallback = 0.0) {
        return fallback;
    }

    inline long long Number(const char* name, long long fallback = 0LL) {
        return fallback;
    }

    inline bool Flag(const char* name, bool fallback = false) {
        return fallback;
    }
}
