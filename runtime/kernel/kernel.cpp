#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>

// Estruturas nativas do ecossistema do PES 2017
struct PPCContext;

namespace kernel {
    void Initialise() {}
    void Shutdown() {}

    // 1. Funções de Kernel com visibilidade aberta exigidas pelo motor gráfico e de mídia
    __attribute__((visibility("default"))) uint64_t SystemTime100ns() { return 0; }
    __attribute__((visibility("default"))) void ReportUnimplemented() {}
    __attribute__((visibility("default"))) void ReportAllocRate() {}
    __attribute__((visibility("default"))) void ReportMemory() {}
    __attribute__((visibility("default"))) void WindowTraceOn() {}
    __attribute__((visibility("default"))) void ReportWaits() {}
    __attribute__((visibility("default"))) void CompleteOverlapped(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) void WindowTrace(char const* a, char const* b, unsigned int c, unsigned int d, char const* e) {}
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const void* p) {}
    __attribute__((visibility("default"))) uint32_t ToPhysical(unsigned int a) { return 0; }
    __attribute__((visibility("default"))) uint32_t FromPhysical(unsigned int a) { return 0; }

    // 2. Mocks adicionais protegidos por linkage fraco contra duplicidades sintáticas
    __attribute__((weak, visibility("default"))) uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    __attribute__((weak, visibility("default"))) void PostNotification(unsigned int a, unsigned int b) {}
    __attribute__((weak, visibility("default"))) const std::filesystem::path& GameRoot() { static std::filesystem::path p; return p; }
    __attribute__((weak, visibility("default"))) PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    __attribute__((weak, visibility("default"))) bool CheckProcessHeap(bool quiet) { return true; }
    __attribute__((weak, visibility("default"))) uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    __attribute__((weak, visibility("default"))) bool FreeGuest(uint32_t address) { return true; }
    __attribute__((weak, visibility("default"))) void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
}

namespace crash {
    __attribute__((visibility("default"))) void RegisterThread(char const* a) {}
    __attribute__((visibility("default"))) void StopHangingMonitor() {}
    __attribute__((visibility("default"))) char const* CurrentThreadName() { return "Main"; }
}

namespace engine {
    __attribute__((visibility("default"))) void ReportPredicateWaits() {}
}

// 3. Forçar as decorações exatas exigidas pelo Linker final (arm64 iOS) via bloco de amarração externa pura
extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN5crash18StopHangingMonitorEv() {}
    __attribute__((visibility("default"))) void _ZN6kernel12AcceptInviteEyyPKcb(uint64_t a, uint64_t b, const char* c, bool d) {}
    __attribute__((visibility("default"))) void _ZN6kernel14MovePlayerDataEyyy(uint64_t a, uint64_t b, uint64_t c) {}
    __attribute__((visibility("default"))) void _ZN6kernel23UpdateTimestampBundleEv() {}
    __attribute__((visibility("default"))) void _ZN6kernel12QueueUserApcEjjjRKNSt3__18functionIFvvEEE(unsigned int a, unsigned int b, unsigned int c, void* d) {}
}
