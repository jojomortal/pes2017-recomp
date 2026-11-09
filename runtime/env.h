#pragma once

#include <cstdlib>

namespace env {

    // Retorna valores fixos e limpos para o iOS passar direto pela largada sem JIT ou sysctl
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
