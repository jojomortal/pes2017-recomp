#pragma once

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <atomic>

namespace diag {
    // Desativa rotinas invasivas de depuração para não quebrar o Sandbox do iOS
    constexpr bool kOn = false;

    inline void Initialise() {}
    inline void Shutdown() {}

    // ESTRUTURA ATÔMICA EXIGIDA PELA GPU (SOLUÇÃO COMPATÍVEL COM ++ E .load())
    struct Stat {
        std::atomic<uint64_t> value{0};

        Stat() = default;
        Stat(uint64_t v) : value(v) {}

        // Sobrecarga de operadores de incremento exigidos pelo command_processor.cpp
        uint64_t operator++(int) { return value.fetch_add(1); }
        Stat& operator++() { value.fetch_add(1); return *this; }
        uint64_t operator+=(uint64_t v) { return value.fetch_add(v); }

        // Método de carregamento seguro de thread exigido nas linhas 1264 e 1268
        uint64_t load() const { return value.load(); }

        // Conversão implícita de tipo para leituras e atribuições diretas
        operator uint64_t() const { return value.load(); }
    };

    struct Counter {
        std::atomic<uint64_t> value{0};

        Counter() = default;
        Counter(uint64_t v) : value(v) {}

        uint64_t operator++(int) { return value.fetch_add(1); }
        Counter& operator++() { value.fetch_add(1); return *this; }
        uint64_t load() const { return value.load(); }

        operator uint64_t() const { return value.load(); }
    };

    // Assinaturas base limpas com argumentos de conformidade do motor original
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
