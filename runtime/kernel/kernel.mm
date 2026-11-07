#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

// Importação das bibliotecas nativas de interface e fundação da Apple (iOS)
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// Variável estática global que guardará o caminho correto descoberto dinamicamente
static std::filesystem::path g_CaminhoJogoValido;
static bool g_ArquivosDoJogoEncontrados = false;

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
            std::string executavelJogo = caminhoPastaPes + "/default.xex";

            // FORÇA A CRIAÇÃO AUTOMÁTICA DA SUBPASTA CORRETA
            std::filesystem::create_directories(caminhoPastaPes);
            
            // Fixa o caminho global para a engine buscar os dados estritamente dentro da subpasta pes/
            g_CaminhoJogoValido = caminhoPastaPes;

            // Verifica se o usuário já colocou o executável do jogo na pasta correta
            if (std::filesystem::exists(executavelJogo)) {
                g_ArquivosDoJogoEncontrados = true;
            }

            // Grava o arquivo de log explicativo na raiz do Documents
            std::ofstream arquivo(caminhoArquivo);
            if (arquivo.is_open()) {
                arquivo << "==================================================\n";
                arquivo << "PES 2017 Launcher Iniciado com Sucesso no iOS!\n";
                arquivo << "==================================================\n\n";
                arquivo << "Status: Pasta 'pes' validada e criada automaticamente.\n";
                arquivo << "Arquivos detectados: " << (g_ArquivosDoJogoEncontrados ? "SIM" : "NAO") << "\n\n";
                arquivo << "Instrucoes: Coloque o seu default.xex limpo (TU0) e as mídias (.cpk) estritamente dentro da pasta 'pes'.\n";
                arquivo.close();
            }
        }
    } catch (...) {}

    // Inicializa uma janela nativa básica
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    UIViewController *viewController = [[UIViewController alloc] init];
    viewController.view.backgroundColor = [UIColor blackColor];
    
    // Injeção de uma label de ajuda na tela caso o usuário abra o app sem os arquivos
    UILabel *labelAviso = [[UILabel alloc] initWithFrame:CGRectMake(20, 100, self.window.bounds.size.width - 40, 200)];
    labelAviso.text = @"PES 2017 Launcher\n\nPor favor, insira os arquivos convertidos do Xbox 360 estritamente dentro da pasta 'pes' usando o aplicativo 'Arquivos' do iPhone ou o iTunes no PC.";
    labelAviso.textColor = [UIColor whiteColor];
    labelAviso.textAlignment = NSTextAlignmentCenter;
    labelAviso.numberOfLines = 0;
    labelAviso.font = [UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
    [viewController.view addSubview:labelAviso];
    
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];

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

// Ponto de entrada padrão limpo em escopo global
__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
    @autoreleasepool {
        // Inicializa a interface da Apple primeiro para disparar os privilégios de arquivos do iOS
        UIApplicationMain(argc, argv, nil, NSStringFromClass([PESAppDelegate class]));
        
        // TRAVA DE SEGURANÇA: Se não encontrar o default.xex na pasta "pes", segura o app ativo na tela de aviso
        if (!g_ArquivosDoJogoEncontrados) {
            extern void CFRunLoopRun(void);
            CFRunLoopRun(); // Prende em loop infinito seguro impedindo o fechamento precoce
        }
        
        return 0;
    }
}
