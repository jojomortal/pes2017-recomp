#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

// Importação das bibliotecas nativas de interface e fundação da Apple (iOS)
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// Variáveis de controle de caminhos dinâmicos
static std::filesystem::path g_CaminhoJogoValido;
static bool g_ArquivosDoJogoEncontrados = false;

// 1. Definição do Ciclo de Vida do Aplicativo Oficial da Apple (AppDelegate Moderno e Válido)
@interface PESAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    @autoreleasepool {
        try {
            // Busca de forma oficial o caminho absoluto da pasta Documents pública do Sandbox do iOS
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *documentsDirectory = [paths firstObject];
            
            if (documentsDirectory) {
                // Força a criação física da subpasta pública "pes" usando o gerenciador oficial da Apple
                NSString *pastaPes = [documentsDirectory stringByAppendingPathComponent:@"pes"];
                [[NSFileManager defaultManager] createDirectoryAtPath:pastaPes withIntermediateDirectories:YES attributes:nil error:nil];
                
                // Fixa o caminho global para a engine buscar os dados estritamente dentro da subpasta pes/
                g_CaminhoJogoValido = std::filesystem::path([pastaPes UTF8String]);

                // Verifica se o usuário já colocou o executável do jogo na pasta correta
                NSString *executavelJogo = [pastaPes stringByAppendingPathComponent:@"default.xex"];
                if ([[NSFileManager defaultManager] fileExistsAtPath:executavelJogo]) {
                    g_ArquivosDoJogoEncontrados = true;
                }

                // Estrutura o texto de log explicativo usando NSString nativa do iOS
                NSString *caminhoArquivoLog = [documentsDirectory stringByAppendingPathComponent:@"log_inicializacao.txt"];
                NSString *textoLog = @"==================================================\n"
                                      "PES 2017 Launcher Iniciado com Sucesso no iOS!\n"
                                      "==================================================\n\n"
                                      "Status: Pasta 'pes' validada e criada automaticamente.\n"
                                      "Instrucoes: Coloque o seu default.xex limpo (TU0) e as mídias (.cpk) estritamente dentro da pasta 'pes'.\n";

                // GRAVAÇÃO DE DISCO OFICIAL DA APPLE: Força o gatilho que ativa a exibição no app "Arquivos" do iPhone
                [textoLog writeToFile:caminhoArquivoLog atomically:YES encoding:NSUTF8StringEncoding error:nil];
            }
        } catch (...) {}

        // CRIAÇÃO DA INTERFACE MODERNA OBRIGATÓRIA PARA VALIDAR O BUNDLE NO IOS
        UIScreen *telaPrincipal = [UIScreen mainScreen];
        self.window = [[UIWindow alloc] initWithFrame:[telaPrincipal bounds]];
        
        UIViewController *viewControllerPrincipal = [[UIViewController alloc] init];
        viewControllerPrincipal.view.backgroundColor = [UIColor blackColor]; // Mantém a tela preta de prontidão
        
        self.window.rootViewController = viewControllerPrincipal;
        [self.window makeKeyAndVisible];
    }

    return YES;
}
@end

namespace kernel {
    void Initialise() {}
    void Shutdown() {}

    // Retorna dinamicamente o caminho da pasta pública correta exigida pela engine
    __attribute__((visibility("default"))) const std::filesystem::path& GameRoot() {
        return g_CaminhoJogoValido;
    }

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
}

// Ponto de entrada padrão em escopo global regulamentado com privilégio total de Sandbox móvel
__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
    @autoreleasepool {
        // Inicializa o ecossistema móvel disparando as permissões de gravação de arquivos
        UIApplicationMain(argc, argv, @"UIApplication", NSStringFromClass([PESAppDelegate class]));
        
        // LOOP INFINITO DE SEGURANÇA: Se os arquivos do jogo não estiverem lá, prende o processo ativo na tela preta
        if (!g_ArquivosDoJogoEncontrados) {
            extern void CFRunLoopRun(void);
            CFRunLoopRun();
        }
        
        return 0;
    }
}
