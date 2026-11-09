#pragma once
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include "diagnostics.h"
#include "env.h"

// DEFINE AS MACROS DE LOG EXIGIDAS PELO CONSOLE.CPP E RESTO DO MOTOR
#define LOGI(...) do { std::printf("[INFO] " __VA_ARGS__); std::printf("\n"); } while(0)
#define LOGW(...) do { std::printf("[WARN] " __VA_ARGS__); std::printf("\n"); } while(0)
#define LOGE(...) do { std::printf("[ERROR] " __VA_ARGS__); std::printf("\n"); } while(0)
#define LOGK(...) do { std::printf("[KERNEL] " __VA_ARGS__); std::printf("\n"); } while(0)

namespace pes17log
{
    inline const char* Stamp()
    {
        static const bool on = diag::Flag("PES17_LOG_TIME");
        if (!on) return "";
        static const auto start = std::chrono::steady_clock::now();
        thread_local char text[24];
        const double seconds = std::chrono::duration<double>(
                                   std::chrono::steady_clock::now() - start).count();
        std::snprintf(text, sizeof text, "%9.3f ", seconds);
        return text;
    }

    inline const std::string& Line(char level, const char* format, va_list arguments)
    {
        thread_local std::string text;
        char prefix[8];
        const int prefixLength = std::snprintf(prefix, sizeof prefix, "[%c] ", level);
        const size_t at = prefixLength > 0 ? size_t(prefixLength) : 0;
        text.assign(prefix, at);

        size_t room = 256;
        for (;;)
        {
            text.resize(at + room);
            va_list copy;
            va_copy(copy, arguments);
            const int written = std::vsnprintf(text.data() + at, room, format, copy);
            va_end(copy);
            if (written < 0) { text.resize(at); return text; }
            if (size_t(written) < room) { text.resize(at + size_t(written)); return text; }
            room = size_t(written) + 1;
        }
    }

    inline std::string Shape(const std::string& line)
    {
        std::string out;
        out.reserve(line.size());
        for (size_t at = 0; at < line.size();)
        {
            const unsigned char c = (unsigned char)line[at];
            if (std::isxdigit(c))
            {
                size_t end = at;
                bool digit = std::isdigit(c) != 0;
                while (end < line.size() && std::isxdigit((unsigned char)line[end]))
                {
                    if (std::isdigit((unsigned char)line[end])) digit = true;
                    end++;
                }
                if (digit) out += '#';
                else out.append(line, at, end - at);
                at = end;
                continue;
            }
            out += char(c);
            at++;
        }
        return out;
    }

    inline const char* Repeat(const std::string& line)
    {
        static const bool all = env::Flag("PES17_LOG_ALL");
        if (all) return "";

        constexpr size_t kKinds = 128;
        struct Entry
        {
            std::string shape;
            uint64_t hits;
        };

        static std::mutex lock;
        static Entry kinds[kKinds];
        static size_t used = 0;

        const std::string shape = Shape(line);

        std::lock_guard guard(lock);
        for (size_t i = 0; i < used; i++)
        {
            if (kinds[i].shape != shape) continue;
            const uint64_t hits = ++kinds[i].hits;
            if ((hits & (hits - 1)) != 0) return nullptr;
            thread_local char suffix[32];
            std::snprintf(suffix, sizeof suffix, "  (×%llu)", (unsigned long long)hits);
            return suffix;
        }
        if (used < kKinds)
        {
            kinds[used].shape = shape;
            kinds[used].hits = 1;
            used++;
        }
        return "";
    }
}

#ifdef PES17_ANDROID
#endif
