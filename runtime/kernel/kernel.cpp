#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>

// Estruturas nativas do ecossistema do PES 2017
struct PPCContext;

namespace kernel {
    void Initialise() {}
    void Shutdown() {}

    // 1. Funções globais básicas com visibilidade default
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

    // 2. Mocks estruturais nativos do ecossistema
    __attribute__((visibility("default"))) uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    __attribute__((visibility("default"))) void PostNotification(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) const std::filesystem::path& GameRoot() { static std::filesystem::path p; return p; }
    __attribute__((visibility("default"))) PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    __attribute__((visibility("default"))) bool CheckProcessHeap(bool quiet) { return true; }
    __attribute__((visibility("default"))) uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    __attribute__((visibility("default"))) bool FreeGuest(uint32_t address) { return true; }
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
}

namespace crash {
    __attribute__((visibility("default"))) void RegisterThread(char const* a) {}
    __attribute__((visibility("default"))) void StopHangingMonitor() {}
    __attribute__((visibility("default"))) char const* CurrentThreadName() { return "Main"; }
}

namespace engine {
    __attribute__((visibility("default"))) void ReportPredicateWaits() {}
}

// 3. O SEGREDO DO SUCESSO: Forçar o apelido do símbolo binário exato exigido pelo Linker (asm)
extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}
    
    // Vinculação cirúrgica com o exato nome que o Linker do Xcode reclamou na tela
    __attribute__((visibility("default"))) void pes_stub_stop_hang() __asm__("_ZN5crash18StopHangingMonitorEv");
    void pes_stub_stop_hang() {}

    __attribute__((visibility("default"))) void pes_stub_accept() __asm__("_ZN6kernel12AcceptInviteEyyPKcb");
    void pes_stub_accept() {}

    __attribute__((visibility("default"))) void pes_stub_move() __asm__("_ZN6kernel14MovePlayerDataEyyy");
    void pes_stub_move() {}

    __attribute__((visibility("default"))) void pes_stub_timestamp() __asm__("_ZN6kernel23UpdateTimestampBundleEv");
    void pes_stub_timestamp() {}

    __attribute__((visibility("default"))) void pes_stub_apc() __asm__("_ZN6kernel12QueueUserApcEjjjRKNSt3__18functionIFvvEEE");
    void pes_stub_apc() {}
}
