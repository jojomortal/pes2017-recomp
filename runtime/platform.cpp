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

extern "C" float roundevenf(float x)
{
    const float nearest = std::round(x);
    if (std::fabs(nearest - x) != 0.5f) return nearest;
    return 2.0f * std::round(x * 0.5f);
}

namespace { DWORD g_mainThread = 0; }

void platform::Initialise()
{
    g_mainThread = GetCurrentThreadId();
    timeBeginPeriod(1);
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
    if (role == ThreadAffinityRole::PrimeCore) mask = (DWORD_PTR(1) << (numCores - 1));
    else if (role == ThreadAffinityRole::PerformanceCore)
    {
        for (DWORD i = numCores / 2; i < numCores; ++i) mask |= (DWORD_PTR(1) << i);
    }
    else if (role == ThreadAffinityRole::WorkerCore)
    {
        for (DWORD i = 0; i < numCores / 2; ++i) mask |= (DWORD_PTR(1) << i);
    }
    if (mask) SetThreadAffinityMask(GetCurrentThread(), mask);
}

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

bool platform::StartThread(void (*body)(void*), void* argument, size_t stackBytes, const char* name)
{
    auto* start = new ThreadStart{ body, argument, name };
    const HANDLE thread = CreateThread(nullptr, stackBytes, &RunThread, start, 0, nullptr);
    if (!thread) { delete start; return false; }
    CloseHandle(thread);
    return true;
}

void* platform::OpenLibrary(const char* path)
{
    const int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    std::wstring wide(size_t(n > 0 ? n : 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wide.data(), n);
    return LoadLibraryExW(wide.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
}

void* platform::LoadedLibrary(const char* name) { return GetModuleHandleA(name); }
void* platform::Symbol(void* library, const char* name) { return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name)); }

bool platform::IsDebuggerPresent() { return false; }

#else

// Implementacoes POSIX genericas para iOS/macOS/Linux
namespace
{
    pthread_t g_appleMainThread;
}

void platform::Initialise()
{
#if defined(__APPLE__)
    g_appleMainThread = pthread_self();
    std::printf("[iOS platform] Inicializando ambiente Apple Core Foundation.\n");
    
    setenv("PES17_DISABLE_SYSCTL_CHECK", "1", 1);
#endif
}

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
    LowerThreadPriority();
}

void platform::SetThreadAffinity(ThreadAffinityRole role) {}

void platform::PrintBacktrace(const char* why)
{
#if !defined(__ANDROID__)
    void* frames[64];
    int n = backtrace(frames, 64);
    std::fprintf(stderr, "\n[E] ---- %s (%d frames) ----\n", why, n);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    std::fprintf(stderr, "[E] --------------------------------\n");
    std::fflush(stderr);
#endif
}

namespace
{
    struct PosixThreadStart { void (*body)(void*); void* argument; const char* thread_name; };
    void* RunPosixThread(void* raw)
    {
        PosixThreadStart start = *static_cast<PosixThreadStart*>(raw);
        delete static_cast<PosixThreadStart*>(raw);
        if (start.thread_name) platform::SetThreadName(start.thread_name);
        start.body(start.argument);
        return nullptr;
    }
}

bool platform::StartThread(void (*body)(void*), void* argument, size_t stackBytes, const char* name)
{
    auto* start = new PosixThreadStart{ body, argument, name };
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackBytes > 0) pthread_attr_setstacksize(&attr, stackBytes);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t thread;
    int res = pthread_create(&thread, &attr, &RunPosixThread, start);
    pthread_attr_destroy(&attr);

    if (res != 0)
    {
        delete start;
        return false;
    }
    return true;
}

void* platform::OpenLibrary(const char* path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
void* platform::LoadedLibrary(const char* name) { return dlopen(nullptr, RTLD_NOW | RTLD_LOCAL); }
void* platform::Symbol(void* library, const char* name) { return dlsym(library, name); }

// Corrigido: Assinatura explicitamente vinculada ao escopo do namespace correto no bloco POSIX
bool platform::IsDebuggerPresent() { return false; }

#endif
