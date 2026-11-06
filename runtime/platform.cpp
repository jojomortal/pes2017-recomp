#include "platform.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <string>
#else
#include <climits>
#include <dlfcn.h>
// bionic has no execinfo.h: Android's unwinder is reached another way, and
// the backtrace below is written by hand there.
#if !defined(__ANDROID__)
#include <execinfo.h>
#endif
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif
#include <unistd.h>
#endif

#ifdef _WIN32

#include <cmath>

// C23's round-half-to-even, which the recompiled vector rounding reaches
// through simde and the UCRT does not have. Whatever the rounding mode: a tie
// goes to the even neighbour, anything else to the nearer one.
extern "C" float roundevenf(float x)
{
    const float nearest = std::round(x);   // ties away from zero
    if (std::fabs(nearest - x) != 0.5f) return nearest;
    return 2.0f * std::round(x * 0.5f);
}

namespace { DWORD g_mainThread = 0; }

void platform::Initialise()
{
    g_mainThread = GetCurrentThreadId();
    timeBeginPeriod(1);
    // A player's build has no console window of its own. Started from a
    // terminal, it writes there, as a console program would.
    if (!GetConsoleWindow() && AttachConsole(ATTACH_PARENT_PROCESS))
    {
        std::freopen("CONOUT$", "w", stdout);
        std::freopen("CONOUT$", "w", stderr);
    }
}

uint32_t platform::ThreadId() { return uint32_t(GetCurrentThreadId()); }
bool platform::IsMainThread() { return GetCurrentThreadId() == g_mainThread; }

void platform::SetThreadName(const char* name)
{
    wchar_t wide[64];
    const int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 64);
    if (n <= 0) return;
    // Windows 10 1607 and later; looked up, so older systems just go without.
    using SetDescription = HRESULT(WINAPI*)(HANDLE, PCWSTR);
    static const auto set = reinterpret_cast<SetDescription>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription")));
    if (set) set(GetCurrentThread(), wide);
}

void platform::LowerThreadPriority() { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); }
void platform::IdleThreadPriority() { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE); }

void platform::SetThreadAffinity(ThreadAffinityRole role)
{
    if (role == ThreadAffinityRole::Default) return;
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    DWORD numCores = sysInfo.dwNumberOfProcessors;
    if (numCores <= 1) return;

    DWORD_PTR mask = 0;
    if (role == ThreadAffinityRole::PrimeCore)
    {
        mask = (DWORD_PTR(1) << (numCores - 1));
    }
    else if (role == ThreadAffinityRole::PerformanceCore)
    {
        for (DWORD i = numCores / 2; i < numCores; ++i)
            mask |= (DWORD_PTR(1) << i);
    }
    else if (role == ThreadAffinityRole::WorkerCore)
    {
        for (DWORD i = 0; i < numCores / 2; ++i)
            mask |= (DWORD_PTR(1) << i);
    }
    if (mask) SetThreadAffinityMask(GetCurrentThread(), mask);
}

// No symbols without a PDB: each frame as its offset in the executable, which
// `llvm-symbolizer --obj=pes17.exe` turns back into a name on the build machine.
void platform::PrintBacktrace(const char* why)
{
    void* frames[64];
    const USHORT n = CaptureStackBackTrace(0, 64, frames, nullptr);
    const auto module = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    std::fprintf(stderr, "\n[E] ---- %s (%u frames) ----\n", why, unsigned(n));
    for (USHORT i = 0; i < n; i++)
    {
        const auto at = reinterpret_cast<uintptr_t>(frames[i]);
        HMODULE owner = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(frames[i]), &owner);
        if (reinterpret_cast<uintptr_t>(owner) == module)
            std::fprintf(stderr, "[E]   exe+0x%llx\n", (unsigned long long)(at - module));
        else
            std::fprintf(stderr, "[E]   %p\n", frames[i]);
    }
    std::fprintf(stderr, "[E] --------------------------------\n");
    std::fflush(stderr);
}

namespace
{
    struct ThreadStart { void (*body)(void*); void* argument; const char* name; };

    DWORD WINAPI RunThread(LPVOID raw)
    {
        ThreadStart start = *static_cast<ThreadStart*>(raw);
        delete static_cast<ThreadStart*>(raw);
        if (start.name) platform::SetThreadName(start.name);
        start.body(start.argument);
        return 0;
    }
}

bool platform::StartThread(void (*body)(void*), void* argument, size_t stackBytes,
                           const char* name)
{
    auto* start = new ThreadStart{ body, argument, name };
    const HANDLE thread = CreateThread(nullptr, stackBytes, &RunThread, start, 0, nullptr);
    if (!thread) { delete start; return false; }
    CloseHandle(thread);   // detached: nothing joins these
    return true;
}

void* platform::OpenLibrary(const char* path)
{
    const int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    std::wstring wide(size_t(n > 0 ? n : 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wide.data(), n);
    // The library's own folder is searched for what it loads in turn.
    return LoadLibraryExW(wide.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
}

void* platform::LoadedLibrary(const char* name) { return GetModuleHandleA(name); }

void* platform::Symbol(void* library, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
}

#else

// Implementações POSIX genéricas para iOS/macOS/Linux
void platform::Initialise() {}

#if defined(__APPLE__)
uint32_t platform::ThreadId() { return uint32_t(pthread_mach_thread_np(pthread_self())); }
bool platform::IsMainThread() { return pthread_main_np() != 0; }
#else
uint32_t platform::ThreadId() { return uint32_t(gettid()); }
bool platform::IsMainThread() { return gettid() == getpid(); }
#endif

void platform::SetThreadName(const char* name)
{
#if defined(__APPLE__)
    pthread_setname_np(name);
#else
    char shortName[16];
    std::snprintf(shortName, sizeof shortName, "%s", name);
    pthread_setname_np(pthread_self(), shortName);
#endif
}

void platform::LowerThreadPriority() 
{ 
#if !defined(__APPLE__)
    setpriority(PRIO_PROCESS, static_cast<id_t>(gettid()), 10); 
#endif
}

void platform::IdleThreadPriority()
{
    sched_param none{};
    if (pthread_setschedparam(pthread_self(), SCHED_IDLE, &none) != 0) LowerThreadPriority();
}

namespace
{
    struct CpuTopology
    {
        uint32_t primeMask = 0;
        uint32_t perfMask = 0;
        uint32_t workerMask = 0;
        bool discovered = false;
    };

    CpuTopology DiscoverCpuTopology()
    {
        CpuTopology topo;
        const long cpus = sysconf(_SC_NPROCESSORS_CONF);
        if (cpus <= 1) return topo;

        const int maxCpus = std::min(int(cpus), 32);
        struct CoreFreq { int id; unsigned long freq; };
        std::vector<CoreFreq> freqs;

        for (int i = 0; i < maxCpus; ++i)
        {
            char path[128];
            std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
            FILE* f = std::fopen(path, "r");
            unsigned long freq = 0;
            if (f)
            {
                if (std::fscanf(f, "%lu", &freq) == 1) freqs.push_back({ i, freq });
                std::fclose(f);
            }
        }

        if (freqs.size() == size_t(maxCpus) && !freqs.empty())
        {
            std::sort(freqs.begin(), freqs.end(), [](const CoreFreq& a, const CoreFreq& b) {
                return a.freq > b.freq;
            });

            unsigned long maxFreq = freqs[0].freq;
            for (const auto& c : freqs)
            {
                if (c.freq >= maxFreq * 95 / 100)
                    topo.primeMask |= (1u << c.id);
                else if (c.freq >= maxFreq * 75 / 100)
                    topo.perfMask |= (1u << c.id);
                else
                    topo.workerMask |= (1u << c.id);
            }
            if (!topo.perfMask) topo.perfMask = topo.primeMask;
            if (!topo.workerMask) topo.workerMask = topo.perfMask;
        }
        else
        {
            if (maxCpus >= 8)
            {
                topo.primeMask = (1u << 7);
                topo.perfMask = (1u << 4) | (1u << 5) | (1u << 6);
                topo.workerMask = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3);
            }
        }
        return topo;
    }
}

void platform::SetThreadAffinity(ThreadAffinityRole role) {}
void platform::PrintBacktrace(const char* why) {}
bool platform::StartThread(void (*body)(void*), void* argument, size_t stackBytes, const char* name) { return false; }
void* platform::OpenLibrary(const char* path) { return nullptr; }
void* platform::LoadedLibrary(const char* name) { return nullptr; }
void* platform::Symbol(void* library, const char* name) { return nullptr; }

#endif
