#pragma once

#include <cstdio>
#include "env.h" // CORREÇÃO CIRÚRGICA: Fornece a declaração do namespace env exigida pelo motor

#define LOGI(...) do { std::printf("[INFO] " __VA_ARGS__); std::printf("\n"); } while(0)
#define LOGW(...) do { std::printf("[WARN] " __VA_ARGS__); std::printf("\n"); } while(0)
#define LOGE(...) do { std::printf("[ERROR] " __VA_ARGS__); std::printf("\n"); } while(0)

namespace log_system {
    inline void Initialise() {}
    inline void Shutdown() {}
}
