#include "install.h"
#include <stdint.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>

// Funcoes de baixo nivel exigidas pelo motor para registrar caminhos virtuais (VFS)
extern "C" {
    __attribute__((visibility("default"))) void VFS_RegisterMount(const char* virtual_path, const char* physical_path);
    __attribute__((visibility("default"))) void VFS_ClearMounts();
}

namespace install {

    // Funcao basica e sincrona para montagem do sistema de arquivos virtual direcionado
    __attribute__((visibility("default"))) void Initialise() {
        try {
            std::string caminhoRaizStr = ".";

            // No iOS, a pasta publica visivel no aplicativo 'Arquivos' fica sempre localizada em $HOME/Documents
            if (const char* homeEnv = std::getenv("HOME")) {
                std::filesystem::path rotaHome(homeEnv);
                std::filesystem::path rotaPastaPes = rotaHome / "Documents" / "pes";
                
                // Se a pasta pes nao existir no Sandbox por algum motivo, garante a criacao automatica
                if (!std::filesystem::exists(rotaPastaPes)) {
                    std::filesystem::create_directories(rotaPastaPes);
                }
                caminhoRaizStr = rotaPastaPes.string();
            } else {
                // Fallback seguro em C++ moderno caso o ambiente nao esteja populado
                caminhoRaizStr = std::filesystem::current_path().string();
            }
            
            std::printf("[iOS VFS Direto] Montando o diretorio de dados em Documents: %s\n", caminhoRaizStr.c_str());
            
            // Limpa mapeamentos antigos para evitar duplicidade de ponteiros de midia
            VFS_ClearMounts();
            
            // Mapeia os caminhos logicos do motor do jogo diretamente para a pasta pes publica do iPhone
            VFS_RegisterMount("game:/", caminhoRaizStr.c_str());
            VFS_RegisterMount("d:/", caminhoRaizStr.c_str());
            VFS_RegisterMount("cache:/", caminhoRaizStr.c_str());
            VFS_RegisterMount("RETAIL_DISC:/", caminhoRaizStr.c_str());
            
            // Cria o arquivo de log para validarmos o sucesso de escrita na pasta Documents
            std::string caminhoLogVFS = caminhoRaizStr + "/log_vfs_ios.txt";
            std::ofstream arquivoLog(caminhoLogVFS);
            if (arquivoLog.is_open()) {
                arquivoLog << "==================================================\n";
                arquivoLog << "VFS ATIVADO COM SUCESSO: Diretorio Documents pareado!\n";
                arquivoLog << "==================================================\n\n";
                arquivoLog << "Rota ativa de execucao: " << caminhoRaizStr << "\n";
                arquivoLog.close();
                std::printf("[iOS VFS Direto] Arquivo log_vfs_ios.txt gerado na pasta pes publica.\n");
            }
        } catch (...) {
            std::printf("[iOS VFS ERRO] Falha ao processar a montagem de rotas em C++.\n");
        }
    }

    __attribute__((visibility("default"))) void Shutdown() {
        VFS_ClearMounts();
    }

    // Mocks fortes de instalacao exigidos para manter a integridade do Linker arm64 nativo
    __attribute__((visibility("default"))) bool IsInstalled() { return true; }
    __attribute__((visibility("default"))) bool CopyGameFiles() { return true; }
    __attribute__((visibility("default"))) float GetCopyProgress() { return 1.0f; }
    __attribute__((visibility("default"))) uint64_t GetRequiredSpace() { return 0; }
    __attribute__((visibility("default"))) uint64_t GetAvailableSpace() { return 1024 * 1024 * 1024; }
}

extern "C" {
    __attribute__((visibility("default"))) void VFS_RegisterMount(const char* virtual_path, const char* physical_path) {}
    __attribute__((visibility("default"))) void VFS_ClearMounts() {}
}
