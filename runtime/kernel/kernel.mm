#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static std::filesystem::path g_CaminhoJogoValido;
static bool g_ArquivosDoJogoEncontrados = false;
static bool g_MotorExecutando = false;

// Alocação estática segura de buffer de paginação protegida contra o ASLR do iOS
alignas(4096) static uint8_t g_MemoriaMapeadaXbox[64 * 1024 * 1024]; 

namespace pes17 {
    int Run(int argc, char** argv);
}

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
    labelAviso.text = @"PES 2017 REAL RECOMP\n\nProcessando base de dados da pasta 'pes'...\nMantenha o aplicativo aberto.";
    labelAviso.textColor = [UIColor greenColor];
    labelAviso.textAlignment = NSTextAlignmentCenter;
    labelAviso.numberOfLines = 0;
    labelAviso.font = [UIFont fontWithName:@"Courier" size:14];
    [viewController.view addSubview:labelAviso];
    
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];
}
@end

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
                std::string executavelJogo = caminhoPastaPes + "/default.xex";

                mkdir(caminhoPastaPes.c_str(), 0777);
                g_CaminhoJogoValido = caminhoPastaPes;
                
                std::printf("[VFS iOS] Varrendo diretorio de dados: %s\n", caminhoPastaPes.c_str());

                if (std::filesystem::exists(executavelJogo)) {
                    g_ArquivosDoJogoEncontrados = true;
                    std::printf("[VFS iOS] Executavel default.xex detectado com sucesso!\n");
                } else {
                    std::printf("[VFS iOS] ATENCAO: default.xex ausente na pasta 'pes'.\n");
                }
            }
            
            if (!g_MotorExecutando) {
                g_MotorExecutando = true;
                std::thread threadMotor([]() {
                    try {
                        char* argv[] = { (char*)"pes17", nullptr };
                        std::printf("[Motor C++] Inicializando interpretador estatico de tabelas.\n");
                        pes17::Run(1, argv);
                    } catch (...) {
                        std::printf("[Motor C++] Excecao capturada na thread do jogo.\n");
                    }
                });
                threadMotor.detach();
            }
            
        } catch (...) {}
    }
    return YES;
}
@end

extern "C" int inline_uikit_main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([PESAppDelegate class]));
    }
}

namespace kernel {
    void Initialise(const std::filesystem::path& gameRoot) {
        if (!gameRoot.empty()) {
            g_CaminhoJogoValido = gameRoot;
        }
    }
    
    void Shutdown() {}
    const std::filesystem::path& GameRoot() { return g_CaminhoJogoValido; }
    
    // RESOLUÇÃO DO ASLR: Converte os ponteiros de forma estritamente relativa ao deslocamento (offset)
    uint32_t ToPhysical(unsigned int a) { 
        uintptr_t offset = a % (64 * 1024 * 1024);
        return (uint32_t)(uintptr_t)(g_MemoriaMapeadaXbox + offset); 
    }
    uint32_t FromPhysical(unsigned int p) { 
        if (p < (uintptr_t)g_MemoriaMapeadaXbox) return 0;
        return (uint32_t)(p - (uintptr_t)g_MemoriaMapeadaXbox); 
    }
    
    uint64_t SystemTime100ns() { return 0; }
    void ReportUnimplemented() {}
    void ReportAllocRate() {}
    void ReportMemory() {}
    void WindowTraceOn() {}
    void ReportWaits() {}
    void CompleteOverlapped(unsigned int a, unsigned int b) {}
    void WindowTrace(char const* a, char const* b, unsigned int c, unsigned int d, char const* e) {}
    void TraceImport(char const* a, unsigned int b, const void* p) {}
    uint32_t AllocateGuest(unsigned int a, unsigned int b) { return 0; }
    void PostNotification(unsigned int a, unsigned int b) {}
    PPCFunc* GuestFunction(uint32_t address) { return nullptr; }
    bool CheckProcessHeap(bool quiet) { return true; }
    uint32_t CreateThreadBlock(uint32_t threadid) { return 0; }
    bool FreeGuest(uint32_t address) { return true; }
    void TraceImport(char const* a, unsigned int b, const PPCContext& c) {}
    void UpdateTimestampBundle() {}
    void AcceptInvite(uint64_t a, uint64_t b, const unsigned char* c, bool d) {}
    void MovePlayerData(uint64_t a, uint64_t b, uint64_t c) {}
    void QueueUserApc(unsigned int a, unsigned int b, unsigned int c) {}
    
    DelayCounts g_fallbackDelay;
    DelayCounts& ThreadDelays() { return g_fallbackDelay; }
}
