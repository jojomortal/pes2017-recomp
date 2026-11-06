#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>

// Estruturas nativas do ecossistema do PES 2017
struct PPCContext;

namespace kernel {
    // Mantendo as inicializações base do projeto
    void Initialise() {}
    void Shutdown() {}

    // 1. Funções globais exigidas pelo Linker final do iOS
    __attribute__((visibility("default"))) uint64_t SystemTime100ns() { return 0; }
    __attribute__((visibility("default"))) void ReportUnimplemented() {}
    __attribute__((visibility("default"))) void ReportAllocRate() {}
    __attribute__((visibility("default"))) void ReportMemory() {}
    __attribute__((visibility("default"))) void WindowTraceOn() {}
    __attribute__((visibility("default"))) void ReportWaits() {}
    __attribute__((visibility("default"))) void CompleteOverlapped(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) void WindowTrace(char const* a, char const* b, unsigned int c, unsigned int d, char const* e) {}
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const void* p) {}
    __attribute__((visibility("default"))) void AcceptInvite(unsigned long long a, unsigned long long b, char const* c, bool d) {}
    __attribute__((visibility("default"))) uint32_t ToPhysical(unsigned int a) { return 0; }
    __attribute__((visibility("default"))) uint32_t FromPhysical(unsigned int a) { return 0; }

    // 2. Mocks de Kernel com as assinaturas exatas e tipos de dados do arquivo kernel.h
    __attribute__((weak)) uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    __attribute__((weak)) void MovePlayerData(unsigned long long a, unsigned long long b, unsigned long long c) {}
    __attribute__((weak)) void PostNotification(unsigned int a, unsigned int b) {}
    __attribute__((weak)) void UpdateTimestampBundle() {}
    __attribute__((weak)) const std::filesystem::path& GameRoot() { static std::filesystem::path p; return p; }

    // Correções cirúrgicas de tipos (casando 100% com o tipo de retorno PPCFunc* exigido pelo Clang)
    __attribute__((weak)) PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    __attribute__((weak)) bool CheckProcessHeap(bool quiet) { return true; }
    __attribute__((weak)) uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    __attribute__((weak)) bool FreeGuest(uint32_t address) { return true; }
}

namespace crash {
    __attribute__((visibility("default"))) void RegisterThread(char const* a) {}
    __attribute__((visibility("default"))) void StopHangingMonitor() {}
    __attribute__((visibility("default"))) char const* CurrentThreadName() { return "Main"; }
}

namespace engine {
    __attribute__((visibility("default"))) void ReportPredicateWaits() {}
}

// Vinculação de escopo C puro exigida para amostragem de frames da GPU
extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}
}
