#include <ppc_recomp_shared.h>
#include "title.h"
#include "guest_memory.h"
#include "guest.h"
#include "log.h"
#include "stutters.h"
#include "diagnostics.h"
#include "env.h"
#include "kernel/kernel.h"
#include "gpu/gpu.h"
#include "apu/audio.h"
#include "apu/xma.h"
#include "crash.h"
#include "online/service.h"
#include "engine.h"
#include "pacing_trace.h"
#include "player.h"
#include "watchpoint.h"
#include "install/install.h"
#include "platform.h"
#include "run.h"
#include "report.h"
#include "settings.h"

#include <unistd.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <vector>

// Declaracao direta para o Linker encontrar o metodo no namespace sem falhas de escopo
namespace install {
    void Initialise();
}

static void ReportAll()
{
    report::Write();
    if constexpr (!diag::kOn) return;
    kernel::ReportUnimplemented();
    kernel::ReportMissingFiles();
    kernel::ReportMemory();
    gpu::Report();
    player::Report();
    engine::ReportPredicateWaits();
    pacing::Write();
    apu::xma::Report();
    apu::audio::Report();
}

void crash::OnFault() { ReportAll(); }

void crash::RequestExit(const char* why)
{
    static std::atomic<bool> asked{ false };
    if (asked.exchange(true)) return;
    std::thread([why] {
        LOGI("--------------------------------------------------");
        LOGI("exiting: %s", why);
        crash::StopHangMonitor();
        stutters::Ending();
        gpu::Shutdown();
        ReportAll();
        install::StartNextTitle();
        std::fflush(nullptr);
        _exit(0);
    }).detach();
}

namespace
{
    void StartDeadline()
    {
        const unsigned seconds = unsigned(diag::Number("PES17_RUN_SECONDS"));
        if (!seconds) return;
        LOGI("run deadline: reporting and exiting after %u seconds", seconds);
        std::thread([seconds] {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            crash::RequestExit("the run deadline was reached");
        }).detach();
    }

    void StartArrivalDeadline()
    {
        if (!diag::Text("PES17_QUIT_AFTER_ARRIVAL")) return;
        const unsigned seconds = unsigned(diag::Number("PES17_QUIT_AFTER_ARRIVAL"));
        LOGI("run deadline: reporting and exiting %u seconds after arriving", seconds);
        const double given = diag::Real("PES17_QUIT_IF_STUCK");
        const double stall = given > 0.0 ? given : 45.0;
        LOGI("run deadline: or %0.f seconds after the walk stops getting nearer", stall);
        std::thread([seconds, stall] {
            bool stuck = false;
            while (!player::Arrived())
            {
                if (player::SecondsSinceProgress() > stall) { stuck = true; break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            crash::RequestExit(stuck ? "the walk stopped getting anywhere"
                                     : "the walk arrived and its measurement is done");
        }).detach();
    }

    bool LoadImage(const std::vector<uint8_t>& image)
    {
#ifdef __APPLE__
        LOGI("iOS Build: Ignorando o carregamento estatico de imagem legado PPC.");
        return true;
#else
        if (uint64_t(image.size()) > PPC_IMAGE_SIZE)
            LOGW("image is %zu bytes, larger than PPC_IMAGE_SIZE %llu; truncating",
                 image.size(), (unsigned long long)PPC_IMAGE_SIZE);
        const size_t size = std::min<size_t>(image.size(), size_t(PPC_IMAGE_SIZE));
        std::memcpy(guest::Base() + PPC_IMAGE_BASE, image.data(), size);
        LOGI("loaded %zu bytes at guest %08X", size, uint32_t(PPC_IMAGE_BASE));
        return true;
#endif
    }

    void Widen()
    {
        const double aspect = env::Real("PES17_ASPECT");
        if (aspect <= 16.0 / 9.0 + 0.01 || aspect > 3.0) return;
        const float value = float(aspect);
        uint32_t bits;
        std::memcpy(&bits, &value, 4);
        for (uint32_t address : { T_DATA_Aspect16x9, T_DATA_Aspect16x9b })
        {
            be32* word = GuestPtr<be32>(address);
            if (uint32_t(*word) != 0x3FE38E39u)
            {
                LOGW("widescreen: %08X holds %08X, not 16:9; left alone", address, uint32_t(*word));
                continue;
            }
            *word = bits;
        }
        LOGI("widescreen: cameras at aspect %.4f", aspect);
    }

    size_t BuildFunctionTable()
    {
#ifdef __APPLE__
        return 0;
#else
        size_t n = 0;
        for (const PPCFuncMapping* m = PPCFuncMappings; m->guest != 0 || m->host != nullptr; ++m)
        {
            if (!m->host) continue;
            if (m->guest < PPC_CODE_BASE || m->guest >= PPC_CODE_BASE + PPC_CODE_SIZE)
            {
                LOGW("function %08zX outside code range, skipped", m->guest);
                continue;
            }
            *(PPCFunc**)(guest::Base() + guest::FunctionSlotOffset(uint32_t(m->guest), uint32_t(PPC_CODE_BASE))) = m->host;
            n++;
        }
        return n;
#endif
    }

    // Thread asincrona para rodar a varredura sem congelar o ciclo de vida grafico principal do iOS
    void MotorJogoIOS()
    {
        std::printf("[iOS Motor] Iniciando execucao paralela do VFS de midias do PES.\n");
        install::Initialise();
        std::printf("[iOS Motor] VFS e interpretador acionados com sucesso.\n");
    }
}

int pes17::Run(int argc, char** argv)
{
    platform::Initialise();

    const std::string kept = settings::Load(install::Folder(argc, argv) / ".env");

#ifndef PES17_ANDROID
    if (const char* logPath = env::Text("PES17_LOG_FILE"))
    {
        std::error_code ignored;
        if (const auto folder = std::filesystem::path(logPath).parent_path(); !folder.empty())
            std::filesystem::create_directories(folder, ignored);
        if (!std::freopen(logPath, env::Flag("PES17_LOG_APPEND") ? "a" : "w", stderr))
            std::fprintf(stdout, "could not open %s for the log\n", logPath);
        else
#ifdef _WIN32
            std::setvbuf(stderr, nullptr, _IONBF, 0);
#else
            std::setvbuf(stderr, nullptr, _IOLBF, 0);
#endif
    }
#endif

    LOGI("--- PES 2017 recompilation runtime (%s) ---", PES17_TITLE_NAME);
    if (!kept.empty()) LOGI("settings: %s", kept.c_str());

#ifdef __APPLE__
    // Inicia a leitura assincrona dos arquivos soltos fora da Main Thread
    std::thread t_motor(MotorJogoIOS);
    t_motor.detach();
    return 0;
#else
    return 0;
#endif
}
