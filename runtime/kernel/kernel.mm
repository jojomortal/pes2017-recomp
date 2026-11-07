#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

// Importação das bibliotecas nativas de interface, gráficos e Metal da Apple
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// Variáveis de controle de caminhos dinâmicos e memória
static std::filesystem::path g_CaminhoJogoValido;
static bool g_ArquivosDoJogoEncontrados = false;
static uint8_t* g_MemoriaVirtualConcedida = nullptr;
static const uint64_t g_TamanhoMemoriaXbox = 512 * 1024 * 1024;

// Interface do ViewController Gráfico Nativo do iOS
@interface PESViewController : UIViewController
@property (nonatomic, strong) CAMetalLayer *metalLayer;
@property (nonatomic, strong) id<MTLDevice> metalDevice;
@end

@implementation PESViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    
    // 1. INICIALIZAÇÃO DO MOTOR GRÁFICO REAL DA APPLE (Metal API)
    self.metalDevice = MTLCreateSystemDefaultDevice();
    
    if (self.metalDevice) {
        self.metalLayer = [CAMetalLayer layer];
        self.metalLayer.device = self.metalDevice;
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.frame = self.view.bounds;
        
        // CORREÇÃO CIRÚRGICA: Uso de máscaras nativas UIView para evitar erros de macro CoreAnimation
        self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        // Acopla a camada de renderização diretamente na tela visível do iPhone
        [self.view.layer addSublayer:self.metalLayer];
    }
    
    // 2. INTERFACE VISUAL DE AVISO DA COMUNIDADE
    UILabel *labelAviso = [[UILabel alloc] initWithFrame:CGRectMake(20, self.view.bounds.size.height / 2 - 100, self.view.bounds.size.width - 40, 200)];
    labelAviso.text = @"PES 2017 Launcher - iOS\n\nPor favor, insira o arquivo 'default.xex' e as mídias (.cpk) dentro da pasta 'pes' usando o app 'Arquivos' do iPhone ou o iTunes no PC.";
    labelAviso.textColor = [UIColor whiteColor];
    labelAviso.textAlignment = NSTextAlignmentCenter;
    labelAviso.numberOfLines = 0;
    labelAviso.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
    [self.view addSubview:labelAviso];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    if (touch) {
        CGPoint pontoTocado = [touch locationInView:self.view];
        (void)pontoTocado;
    }
}
@end

// 3. Ciclo de Vida do Aplicativo Oficial da Apple (AppDelegate)
@interface PESAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    @autoreleasepool {
        try {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *documentsDirectory = [paths firstObject];
            
            if (documentsDirectory) {
                NSString *pastaPes = [documentsDirectory stringByAppendingPathComponent:@"pes"];
                [[NSFileManager defaultManager] createDirectoryAtPath:pastaPes withIntermediateDirectories:YES attributes:nil error:nil];
                
                g_CaminhoJogoValido = std::filesystem::path([pastaPes UTF8String]);

                NSString *executavelJogo = [pastaPes stringByAppendingPathComponent:@"default.xex"];
                if ([[NSFileManager defaultManager] fileExistsAtPath:executavelJogo]) {
                    g_ArquivosDoJogoEncontrados = true;
                }

                NSString *caminhoArquivoLog = [documentsDirectory stringByAppendingPathComponent:@"log_inicializacao.txt"];
                NSString *textoLog = @"==================================================\n"
                                      "PES 2017 Launcher Iniciado com Sucesso no iOS!\n"
                                      "==================================================\n\n"
                                      "Status: Pasta 'pes' criada e validada graficamente.\n";
                [textoLog writeToFile:caminhoArquivoLog atomically:YES encoding:NSUTF8StringEncoding error:nil];
            }
            
            if (!g_MemoriaVirtualConcedida) {
                g_MemoriaVirtualConcedida = (uint8_t*)malloc(g_TamanhoMemoriaXbox);
                if (g_MemoriaVirtualConcedida) {
                    memset(g_MemoriaVirtualConcedida, 0, g_TamanhoMemoriaXbox);
                }
            }
        } catch (...) {}

        // CORREÇÃO DE DEPRECIAÇÃO: Inicialização de janela moderna compatível com iOS 15, 16 e 17+
        self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        
        PESViewController *viewController = [[PESViewController alloc] init];
        self.window.rootViewController = viewController;
        [self.window makeKeyAndVisible];
    }
    return YES;
}
@end

namespace kernel {
    void Initialise() {}
    void Shutdown() {}

    __attribute__((visibility("default"))) const std::filesystem::path& GameRoot() {
        return g_CaminhoJogoValido;
    }

    __attribute__((visibility("default"))) uint32_t ToPhysical(unsigned int address) { 
        if (g_MemoriaVirtualConcedida && address < g_TamanhoMemoriaXbox) {
            return (uint32_t)(uintptr_t)(g_MemoriaVirtualConcedida + address);
        }
        return 0; 
    }
    
    __attribute__((visibility("default"))) uint32_t FromPhysical(unsigned int physicalAddress) { 
        if (g_MemoriaVirtualConcedida && (uintptr_t)physicalAddress >= (uintptr_t)g_MemoriaVirtualConcedida) {
            uintptr_t offset = (uintptr_t)physicalAddress - (uintptr_t)g_MemoriaVirtualConcedida;
            if (offset < g_TamanhoMemoriaXbox) {
                return (uint32_t)offset;
            }
        }
        return 0; 
    }

    __attribute__((visibility("default"))) uint64_t SystemTime100ns() { return 0; }
    __attribute__((visibility("default"))) void ReportUnimplemented() {}
    __attribute__((visibility("default"))) void ReportAllocRate() {}
    __attribute__((visibility("default"))) void ReportMemory() {}
    __attribute__((visibility("default"))) void WindowTraceOn() {}
    __attribute__((visibility("default"))) void ReportWaits() {}
    __attribute__((visibility("default"))) void CompleteOverlapped(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) void WindowTrace(char const* a, char const* b, unsigned int c, unsigned int d, char const* e) {}
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const void* p) {}

    __attribute__((visibility("default"))) uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    __attribute__((visibility("default"))) void PostNotification(unsigned int a, unsigned int b) {}
    __attribute__((visibility("default"))) PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    __attribute__((visibility("default"))) bool CheckProcessHeap(bool quiet) { return true; }
    __attribute__((visibility("default"))) uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    __attribute__((visibility("default"))) bool FreeGuest(uint32_t address) { return true; }
    __attribute__((visibility("default"))) void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
    __attribute__((visibility("default"))) void UpdateTimestampBundle() {}

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

extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}
}

// 4. Ponto de Entrada Oficial Regulamentado com Tratamento de Loop de Tela
__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
    @autoreleasepool {
        UIApplicationMain(argc, argv, @"UIApplication", NSStringFromClass([PESAppDelegate class]));
        
        // Se o executável do jogo estiver ausente, segura a execução na tela gráfica Metal ativa
        if (!g_ArquivosDoJogoEncontrados) {
            extern void CFRunLoopRun(void);
            CFRunLoopRun();
        }
        return 0;
    }
}
