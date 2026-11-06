#include "kernel.h"
#include <stdint.h>
#include <functional>

// Estruturas e namespaces nativos do ecossistema do PES 2017
struct PPCContext;

namespace kernel {
    // Funções base de simulação existentes do projeto
    void Initialise() {}
    void Shutdown() {}

    // Stubs e Mocks adicionados para conformidade do Launcher Limpo no iOS
    __attribute__((visibility("default"))) uint64_t SystemTime100ns() { return 0; }
    __attribute__((visibility("default"))) void* CheckProcessHeap(void* p) { return p; }
    __attribute__((visibility("default"))) void* CheckProcessHeap(bool p) { return nullptr; }
    __attribute__((visibility("default"))) void PostNotification(uint32_t a, uint32_t b) {}
    __attribute__((visibility("default"))) void ReportUnimplemented() {}
    __attribute__((visibility("default"))) void UpdateTimestampBundle() {}
    __attribute__((visibility("default"))) uint32_t GameRoot() { return 0; }
    __attribute__((visibility("default"))) void FreeGuest(uint32_t a) {}
    __attribute__((visibility("default"))) void GuestFunction(uint32_t a) {}
    __attribute__((visibility("default"))) void WindowTraceOn() {}
    __attribute__((visibility("default"))) void MovePlayerData(uint64_t a, uint64_t b, uint64_t c) {}
    __attribute__((visibility("default"))) void MovePlayerData(unsigned long long a, unsigned long long b, unsigned long long c) {}
    __attribute__((visibility("default"))) void ReportAllocRate() {}
    __attribute__((visibility("default"))) void ReportMemory() {}
    __attribute__((visibility("default"))) uint32_t AllocateGuest(uint32_t a, uint32_t b) { return 0; }
    __attribute__((visibility("default"))) void QueueUserApc(uint32_t a, uint32_t b, uint32_t c, const std::function<void()>& d) {}
    __attribute__((visibility("default"))) void QueueUserApc(unsigned int a, unsigned int b, unsigned int c, std::function<void()> const& d) {}
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const void* c) {}
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
    __attribute__((visibility("default"))) void WindowTrace(char const* a, char const* b, unsigned int c, unsigned int d, char const* e) {}
    __attribute__((visibility("default"))) void AcceptInvite(unsigned long long a, unsigned long long b, char const* c, bool d) {}
    __attribute__((visibility("default"))) void AcceptInvite(unsigned long long a, unsigned long long b, unsigned char const* c, bool d) {}
    __attribute__((visibility("default"))) uint32_t FromPhysical(unsigned int a) { return 0; }
    __attribute__((visibility("default"))) void CreateThreadBlock(unsigned int a) {}
    __attribute__((visibility("default"))) void CompleteOverlapped(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) uint32_t ToPhysical(unsigned int a) { return 0; }
    __attribute__((visibility("default"))) void ReportWaits() {}
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
