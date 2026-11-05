#ifndef __APPLE__
#include "recorder.h"
#ifndef __APPLE__
// =========================================================================
// BLOCO SE NÃO FOR APPLE (ANDROID / PC ORIGINAL)
// =========================================================================
#include "../../diagnostics.h"
#include "../../crash.h"
#include "../../env.h"
#include "../../log.h"
#include "../../platform.h"
#include "../../report.h"
#ifdef PES17_ANDROID
#include "../../android/android.h"
#endif

#include <atomic>
#include <thread>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define PES17_SPIN_HINT() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define PES17_SPIN_HINT() __asm__ __volatile__("yield" ::: "memory")
#else
#define PES17_SPIN_HINT() ((void)0)
#endif

namespace vk::record::detail
{
    bool threaded = false;
    VkCommandBuffer into = VK_NULL_HANDLE;
}

namespace
{
    using vk::record::detail::Header;
    constexpr uint64_t kBytes = 16u << 20;
    constexpr uint32_t kSpins = 4000;

    struct Queue
    {
        uint8_t* ring = nullptr;
        uint64_t reserved = 0;
        uint64_t madeSeen = 0;
        uint64_t roomWaits = 0, waits = 0;
        alignas(64) std::atomic<uint64_t> written{ 0 };
        std::atomic<bool> recorderAsleep{ false };
        alignas(64) std::atomic<uint64_t> made{ 0 };
        std::atomic<uint32_t> waiters{ 0 };
        alignas(64) bool stopping = false;
        std::atomic<uint64_t> calls{ 0 }, sleeps{ 0 };
        std::thread thread;
    };
    Queue q;

    void Run()
    {
        crash::RegisterThread("recorder");
        platform::SetThreadAffinity(platform::ThreadAffinityRole::PerformanceCore);
#ifdef PES17_ANDROID
        android::perf::AddHintThread(true);
#endif
        uint8_t* const ring = q.ring;
        uint64_t at = q.made.load(std::memory_order_relaxed);
        uint64_t calls = 0;
        while (!q.stopping)
        {
            const uint64_t end = q.written.load(std::memory_order_acquire);
            if (at == end)
            {
                q.calls.store(calls, std::memory_order_relaxed);
                bool more = false;
                for (uint32_t i = 0; i < kSpins && !more; i++)
                {
                    PES17_SPIN_HINT();
                    more = q.written.load(std::memory_order_relaxed) != at;
                }
                if (more) continue;
                q.recorderAsleep.store(true, std::memory_order_seq_cst);
                if (q.written.load(std::memory_order_seq_cst) == at)
                {
                    q.sleeps.fetch_add(1, std::memory_order_relaxed);
                    q.written.wait(at, std::memory_order_seq_cst);
                }
                q.recorderAsleep.store(false, std::memory_order_relaxed);
                continue;
            }
            while (at != end)
            {
                Header* header = reinterpret_cast<Header*>(ring + (at & (kBytes - 1)));
                if (header->run) header->run(header + 1, header->command);
                at += header->bytes;
                calls++;
                q.made.store(at, std::memory_order_seq_cst);
                if (q.waiters.load(std::memory_order_seq_cst)) q.made.notify_all();
            }
        }
        q.calls.store(calls, std::memory_order_relaxed);
    }

    void WaitMade(uint64_t position)
    {
        if (q.made.load(std::memory_order_acquire) >= position) return;
        for (uint32_t i = 0; i < kSpins; i++)
        {
            PES17_SPIN_HINT();
            if (q.made.load(std::memory_order_acquire) >= position) return;
        }
        q.waiters.fetch_add(1, std::memory_order_seq_cst);
        for (;;)
        {
            const uint64_t now = q.made.load(std::memory_order_seq_cst);
            if (now >= position) break;
            q.made.wait(now, std::memory_order_seq_cst);
        }
        q.waiters.fetch_sub(1, std::memory_order_relaxed);
    }
}

void* vk::record::detail::Reserve(uint32_t bytes)
{
    uint64_t at = q.reserved;
    uint64_t offset = at & (kBytes - 1);
    const uint64_t pad = offset + bytes > kBytes ? kBytes - offset : 0;
    const uint64_t end = at + pad + bytes;
    if (end - q.madeSeen > kBytes)
    {
        q.madeSeen = q.made.load(std::memory_order_acquire);
        if (end - q.madeSeen > kBytes)
        {
            q.roomWaits++;
            report::Timed spent(report::kRecorderWait);
            WaitMade(end - kBytes);
            q.madeSeen = q.made.load(std::memory_order_acquire);
        }
    }
    if (pad)
    {
        new (q.ring + offset) Header{ nullptr, VK_NULL_HANDLE, uint32_t(pad) };
        at += pad;
        offset = 0;
    }
    q.reserved = at + bytes;
    return q.ring + offset;
}

void vk::record::detail::Publish()
{
    q.written.store(q.reserved, std::memory_order_seq_cst);
    if (q.recorderAsleep.load(std::memory_order_seq_cst)) q.written.notify_one();
}

void vk::record::Start()
{
    if (detail::threaded) return;
    static const bool off =
        diag::kOn && diag::Text("PES17_RECORD_THREAD") && !diag::Flag("PES17_RECORD_THREAD");
    if (off)
    {
        LOGI("recorder: PES17_RECORD_THREAD=0, the consumer records its own commands");
        return;
    }
    q.ring = static_cast<uint8_t*>(::operator new(kBytes, std::align_val_t(64)));
    q.reserved = q.madeSeen = 0;
    q.written.store(0);
    q.made.store(0);
    q.stopping = false;
    detail::threaded = true;
    q.thread = std::thread(Run);
}

void vk::record::Stop()
{
    if (!detail::threaded) return;
    Host([] { q.stopping = true; });
    q.thread.join();
    detail::threaded = false;
    ::operator delete(q.ring, std::align_val_t(64));
    q.ring = nullptr;
}

bool vk::record::Threaded() { return detail::threaded; }

void vk::record::Into(VkCommandBuffer command) { detail::into = command; }

uint64_t vk::record::Position()
{
    return detail::threaded ? q.written.load(std::memory_order_acquire) : 0;
}

bool vk::record::Done(uint64_t position)
{
    return !detail::threaded || q.made.load(std::memory_order_acquire) >= position;
}

void vk::record::WaitFor(uint64_t position)
{
    if (Done(position)) return;
    q.waits++;
    report::Timed spent(report::kRecorderWait);
    WaitMade(position);
}

void vk::record::Drain() { WaitFor(Position()); }

void vk::record::Report()
{
    const uint64_t calls = q.calls.load();
    if (!calls) return;
    LOGI("recorder: %llu calls made on its own thread; it slept %llu times; the consumer"
         " waited for it %llu times, %llu of them for room in the queue",
         (unsigned long long)calls, (unsigned long long)q.sleeps.load(),
         (unsigned long long)(q.waits + q.roomWaits), (unsigned long long)q.roomWaits);
}
#endif
#endif // __APPLE__
