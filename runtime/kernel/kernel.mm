#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread> // Inclusão mestre para controle de Threads nativas em C++

// Importações nativas do ecossistema Apple móvel
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// Cabeçalhos POSIX de baixo nível para criação cirúrgica de pastas
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static std::filesystem::path g_CaminhoJogoValido;
static bool g_ArquivosDoJogoEncontrados = false;
static bool g_MotorExecutando = false;
static uint8_t* g_MemoriaVirtualConcedida = nullptr;
static const uint64_t g_TamanhoMemoriaXbox = 512 * 1024 * 1024;

// Declaração externa da função do compilador que inicia a leitura mestre do jogo
extern "C" void loadFromGuest();

// 1. Definição do SceneDelegate exigido pelas versões modernas do iOS
@interface PESSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    UIViewController *viewController = [[UIViewController alloc] init];
    viewController.view.backgroundColor = [UIColor blackColor];
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        CAMetalLayer *metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.frame = viewController.view.bounds;
        viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [viewController.view.layer addSublayer:metalLayer];
    }
    
    UILabel *labelAviso = [[UILabel alloc] initWithFrame:CGRectMake(20, viewController.view.bounds.size.height / 2 - 100, viewController.view.bounds.size.width - 40, 200)];
    labelAviso.text = @"PES 2017 Launcher\n\nCarregando os arquivos da pasta 'pes'...";
    labelAviso.textColor = [UIColor whiteColor];
    labelAviso.textAlignment = NSTextAlignmentCenter;
    labelAviso.numberOfLines = 0;
    labelAviso.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
    [viewController.view addSubview:labelAviso];
    
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];
}
@end

// 2. AppDelegate que gerencia o ciclo inicial e a criação forçada da pasta pes/
@interface PESAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation PESAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    @autoreleasepool {
        try {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *documentsDirectory = [paths firstObject];
            
            if (documentsDirectory) {
                std::string caminhoDocuments = [documentsDirectory UTF8String];
                std::string caminhoPastaPes = caminhoDocuments + "/pes";
                std::string caminhoArquivoLog = caminhoDocuments + "/log_inicializacao.txt";
                std::string executavelJogo = caminhoPastaPes + "/default.xex";

                mkdir(caminhoPastaPes.c_str(), 0777);
                
                std::ofstream arquivo(caminhoArquivoLog);
                if (arquivo.is_open()) {
                    arquivo << "==================================================\n";
                    arquivo << "PES 2017 Launcher Ativo via Modelo Sonic 2006!\n";
                    arquivo << "==================================================\n";
                    arquivo.close();
                }
                
                g_CaminhoJogoValido = caminhoPastaPes;
                if (std::filesystem::exists(executavelJogo)) {
                    g_ArquivosDoJogoEncontrados = true;
                }
            }
            
            if (!g_MemoriaVirtualConcedida) {
                g_MemoriaVirtualConcedida = (uint8_t*)malloc(g_TamanhoMemoriaXbox);
                if (g_MemoriaVirtualConcedida) {
                    memset(g_MemoriaVirtualConcedida, 0, g_TamanhoMemoriaXbox);
                }
            }
            
            // DISPARO AS SÍNCRONO DA PORTABILIDADE: 
            // Se os arquivos existirem, abre uma thread paralela dedicada para o motor de leitura trabalhar sem travar a interface da Apple
            if (g_ArquivosDoJogoEncontrados && !g_MotorExecutando) {
                g_MotorExecutando = true;
                std::thread threadMotor([]() {
                    try {
                        loadFromGuest(); // Invoca a leitura em segundo plano do default.xex
                    } catch (...) {}
                });
                threadMotor.detach(); // Libera a thread para rodar solta em background
            }
            
        } catch (...) {}
    }
    return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}
@end

// GATILHOS DO GUEST
extern "C" {
    __attribute__((visibility("default"))) void loadFromGuest() {}
    __attribute__((visibility("default"))) void _Z13loadFromGuestv() {}
}

namespace kernel {
    void Initialise() {}
    void Shutdown() {}
    __attribute__((visibility("default"))) const std::filesystem::path& GameRoot() { return g_CaminhoJogoValido; }
    __attribute__((visibility("default"))) uint32_t ToPhysical(unsigned int a) { return g_MemoriaVirtualConcedida ? (uint32_t)(uintptr_t)(g_MemoriaVirtualConcedida + a) : 0; }
    __attribute__((visibility("default"))) uint32_t FromPhysical(unsigned int p) { return g_MemoriaVirtualConcedida ? (uint32_t)((uintptr_t)p - (uintptr_t)g_MemoriaVirtualConcedida) : 0; }
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

namespace engine { void ReportPredicateWaits() {} }

extern "C" {
    __attribute__((visibility("default"))) void SampleCountEvent(unsigned int a) {}
    __attribute__((visibility("default"))) void _ZN12_GLOBAL__N_116SampleCountEventEj(unsigned int a) {}
}

__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, @"UIApplication", NSStringFromClass([PESAppDelegate class]));
    }
}
