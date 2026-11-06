#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

// Importa a biblioteca nativa de ciclo de vida e caminhos do ecossistema Apple
extern "C" {
    void CFRunLoopRun(void);
    // Funções nativas da Apple para localizar pastas do Sandbox de forma limpa
    char* NSHomeDirectory(void);
}

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

    // 2. Mocks consolidados como definicoes fortes para o Linker arm64 vincular de forma garantida
    __attribute__((visibility("default"))) uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    __attribute__((visibility("default"))) void PostNotification(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) const std::filesystem::path& GameRoot() { static std::filesystem::path p; return p; }
    __attribute__((visibility("default"))) PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    __attribute__((visibility("default"))) bool CheckProcessHeap(bool quiet) { return true; }
    __attribute__((visibility("default"))) uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    __attribute__((visibility("default"))) bool FreeGuest(uint32_t address) { return true; }
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
    __attribute__((visibility("default"))) void UpdateTimestampBundle() {}

    // Assinaturas exatas mapeadas com linkage forte para os Mangled Names do ecossistema Apple
    __attribute__((visibility("default"))) void AcceptInvite(uint64_t a, uint64_t b, const unsigned char* c, bool d) {}
    __attribute__((visibility("default"))) void MovePlayerData(uint64_t a, uint64_t b, uint64_t c) {}
    __attribute__((visibility("default"))) void QueueUserApc(unsigned int a, unsigned int b, unsigned int c, const std::function<void()>& d) {}
}

namespace crash {
    __attribute__((visibility("default"))) void RegisterThread(char const* a) {}
    __attribute__((visibility("default"))) void StopHangingMonitor() {}
    __attribute__((visibility("default"))) char const* CurrentThreadName() { return "Main"; }
}

namespace engine {
    __attribute__((visibility("default"))) void ReportPredicateWaits() {}
}

// 3. Vinculações puras remanescentes de escopo C exigidas pela GPU e ponto de entrada da Apple
extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}

    // O main oficial que localiza a pasta Documents real do iPhone e força a gravação física
    __attribute__((visibility("default"))) int main(int argc, char* argv[]) {
        try {
            // Busca o caminho base do diretório Home do App no iPhone
            char* homeDir = NSHomeDirectory();
            if (homeDir != nullptr) {
                std::string caminhoDocuments = std::string(homeDir) + "/Documents";
                std::string caminhoArquivo = caminhoDocuments + "/log_inicializacao.txt";
                std::string caminhoPastaPes = caminhoDocuments + "/pes";

                // Força a criação automática da subpasta "pes" caso ela não exista
                std::filesystem::create_directories(caminhoPastaPes);

                // Grava o arquivo texto de ativação diretamente no Documents visível do usuário
                std::ofstream arquivo(caminhoArquivo);
                if (arquivo.is_open()) {
                    arquivo << "==================================================\n";
                    arquivo << "PES 2017 Launcher Iniciado com Sucesso no iOS!\n";
                    arquivo << "==================================================\n\n";
                    arquivo << "Status: Aguardando injeção de arquivos do jogo.\n";
                    arquivo << "Instruções: Insira o default.xex convertido e a database (.cpk/.bin) dentro da pasta 'pes' gerada aqui ao lado.\n";
                    arquivo.close();
                }
            }
        } catch (...) {}

        // Prende o aplicativo aberto na tela preta estável para dar tempo do iTunes/iOS ler as chaves
        CFRunLoopRun();
        return 0;
    }
}
