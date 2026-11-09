#include "kernel.h"
#include <stdint.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// Frameworks graficos e de sistema homologados para aplicacoes nativas iOS
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// APIs POSIX nativas para gerenciamento de pastas e Sandbox
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static std::filesystem::path g_CaminhoBaseDocumentos;
static bool g_IniciadorAtivo = false;

namespace pes17 {
    int Run(int argc, char** argv);
}

// Interface do Controlador de Tela Nativo em Objective-C++
@interface PESMainViewController : UIViewController
@end

@implementation PESMainViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    
    id<MTLDevice> dispositivoMetal = MTLCreateSystemDefaultDevice();
    if (dispositivoMetal) {
        CAMetalLayer *camadaGrafica = [CAMetalLayer layer];
        camadaGrafica.device = dispositivoMetal;
        camadaGrafica.pixelFormat = MTLPixelFormatBGRA8Unorm;
        camadaGrafica.frame = self.view.bounds;
        [self.view.layer addSublayer:camadaGrafica];
    }
    
    UILabel *rotuloStatus = [[UILabel alloc] initWithFrame:CGRectMake(20, self.view.bounds.size.height / 2 - 80, self.view.bounds.size.width - 40, 160)];
    rotuloStatus.text = @"PES 17 MÓVEL\n\nSincronizando tabelas do VFS de console...\nMantenha o aplicativo aberto.";
    rotuloStatus.textColor = [UIColor greenColor];
    rotuloStatus.textAlignment = NSTextAlignmentCenter;
    rotuloStatus.numberOfLines = 0;
    rotuloStatus.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:15];
    [self.view addSubview:rotuloStatus];
}
@end

// Delegado de Cena obrigatorio para conformidade visual com o iOS
@interface PESMainSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation PESMainSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.rootViewController = [[PESMainViewController alloc] init];
    [self.window makeKeyAndVisible];
}
@end

// Delegado Principal da Aplicacao responsavel pelo bypass de Sandbox
@interface PESMainAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation PESMainAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Deléga a inicializacao síncrona do VFS em segundo plano mantendo a interface ativa em primeiro plano
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        @autoreleasepool {
            try {
                NSArray *diretorios = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
                NSString *caminhoDocsApple = [diretorios firstObject];
                
                if (caminhoDocsApple) {
                    std::string stringDocs = [caminhoDocsApple UTF8String];
                    std::string subPastaPes = stringDocs + "/pes";
                    
                    // Força a criacao fisica da pasta publica visivel no app 'Arquivos'
                    mkdir(subPastaPes.c_str(), 0777);
                    g_CaminhoBaseDocumentos = subPastaPes;
                }
                
                if (!g_IniciadorAtivo) {
                    g_IniciadorAtivo = true;
                    char* argumentosFalsos[] = { (char*)"pes17", nullptr };
                    pes17::Run(1, argumentosFalsos);
                }
            } catch (...) {}
        }
    });
    return YES;
}
@end

// Ponto de entrada nativo exportado para o main.cpp global
extern "C" int inline_uikit_main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([PESMainAppDelegate class]));
    }
}

// Abstracoes de Kernel exigidas pelas tabelas de recompilacao estatica do projeto
namespace kernel {
    void Initialise(const std::filesystem::path& gameRoot) {
        if (!gameRoot.empty()) {
            g_CaminhoBaseDocumentos = gameRoot;
        }
    }
    
    void Shutdown() {}
    const std::filesystem::path& GameRoot() { return g_CaminhoBaseDocumentos; }
    
    // Alocação heap dinamicamente isolada para passar limpa pelas tabelas do Trustd sem JIT
    uint32_t ToPhysical(unsigned int a) { 
        static std::vector<uint8_t>* bufferSeguroXbox = new std::vector<uint8_t>(512 * 1024, 0);
        uintptr_t baseEndereco = (uintptr_t)bufferSeguroXbox->data();
        uintptr_t deslocamentoRelative = a % (512 * 1024);
        return (uint32_t)(baseEndereco + deslocamentoRelative);
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
    
    DelayCounts temporizadorFallback;
    DelayCounts& ThreadDelays() { return temporizadorFallback; }
}
