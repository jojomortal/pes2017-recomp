#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

// Importação das bibliotecas nativas de interface e fundação da Apple (iOS)
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// 1. Definição do Ciclo de Vida do Aplicativo Oficial da Apple (AppDelegate)
@interface PESAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    try {
        // Busca de forma oficial o caminho absoluto da pasta Documents pública do Sandbox do iOS
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = [paths firstObject];
        
        if (documentsDirectory) {
            std::string caminhoDocuments = [documentsDirectory UTF8String];
            std::string caminhoPastaPes = caminhoDocuments + "/pes";
            std::string caminhoArquivo = caminhoDocuments + "/log_inicializacao.txt";

            // FORÇA A CRIAÇÃO AUTOMÁTICA DA PASTA VISÍVEL PARA O USUÁRIO NO APP "ARQUIVOS"
            std::filesystem::create_directories(caminhoPastaPes);

            // Grava o arquivo de log para garantir que o iOS atualize o índice de arquivos do sistema
            std::ofstream arquivo(caminhoArquivo);
            if (arquivo.is_open()) {
                arquivo << "==================================================\n";
                arquivo << "PES 2017 Launcher Iniciado com Sucesso no iOS!\n";
                arquivo << "==================================================\n\n";
                arquivo << "Status: Aguardando injecao de arquivos do jogo.\n";
                arquivo << "Instrucoes: Coloque o arquivo default.xex e as databases dentro da pasta 'pes' criada automaticamente aqui ao lado.\n";
                arquivo.close();
            }
        }
    } catch (...) {}

    // Inicializa uma janela nativa básica vazia para segurar o app aberto sem dar crash precoce
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    UIViewController *viewController = [[UIViewController alloc] init];
    viewController.view.backgroundColor = [UIColor blackColor]; // Mantém a tela preta estável de prontidão
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];

    return YES;
}
@end

namespace kernel {
    void Initialise() {}
    void Shutdown() {}

    // Funções de Kernel com visibilidade aberta exigidas pelo motor gráfico e de mídia
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

    // Mocks consolidados como definições fortes para o Linker arm64 vincular de forma garantida
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

// 2. Vinculações de escopo C puro exigidas pela GPU e ponto de entrada oficial da Apple
extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}

    // BLINDAGEM DO MAIN: Escopo C puro forçado extern C para o Linker arm64 encontrar o símbolo _main nativo
    __attribute__((visibility("default"))) int main(int argc, char* argv[]) {
        @autoreleasepool {
            return UIApplicationMain(argc, argv, nil, NSStringFromClass([PESAppDelegate class]));
        }
    }
}
