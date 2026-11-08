#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

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

namespace pes17 {
    int Run(int argc, char** argv);
}

@interface PESViewController : UIViewController
@end

@implementation PESViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        CAMetalLayer *metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.frame = self.view.bounds;
        [self.view.layer addSublayer:metalLayer];
    }
    
    UILabel *labelAviso = [[UILabel alloc] initWithFrame:CGRectMake(20, self.view.bounds.size.height / 2 - 100, self.view.bounds.size.width - 40, 200)];
    labelAviso.text = @"PES 2017 iOS RECOMP\n\nInicializando o motor grafico nativo...\nAguardando leitura da pasta 'pes'.";
    labelAviso.textColor = [UIColor greenColor];
    labelAviso.textAlignment = NSTextAlignmentCenter;
    labelAviso.numberOfLines = 0;
    labelAviso.font = [UIFont fontWithName:@"Courier" size:16];
    [self.view addSubview:labelAviso];
}
@end

@interface PESSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.rootViewController = [[PESViewController alloc] init];
    [self.window makeKeyAndVisible];
}
@end

@interface PESAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation PESAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // DISPARO SEGURO POSTERIOR: Garante a estabilização completa da janela gráfica do iOS
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        @autoreleasepool {
            try {
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
                NSString *documentsDirectory = [paths firstObject];
                
                if (documentsDirectory) {
                    std::string caminhoDocuments = [documentsDirectory UTF8String];
                    std::string caminhoPastaPes = caminhoDocuments + "/pes";
                    mkdir(caminhoPastaPes.c_str(), 0777);
                    g_CaminhoJogoValido = caminhoPastaPes;
                }
                
                if (!g_MotorExecutando) {
                    g_MotorExecutando = true;
                    char* argv[] = { (char*)"pes17", nullptr };
                    pes17::Run(1, argv);
                }
            } catch (...) {}
        }
    });
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
    
    // Alocação heap isolada reduzida protegida contra verificadores estáticos do Kernel da Apple
    uint32_t ToPhysical(unsigned int a) { 
        static std::vector<uint8_t>* g_MesaVirtualXbox = new std::vector<uint8_t>(256 * 1024, 0);
        uintptr_t basePtr = (uintptr_t)g_MesaVirtualXbox->data();
        uintptr_t offset = a % (256 * 1024);
        return (uint32_t)(basePtr + offset);
    }
    
    uint32_t FromPhysical(unsigned int p) { return 0; }
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
