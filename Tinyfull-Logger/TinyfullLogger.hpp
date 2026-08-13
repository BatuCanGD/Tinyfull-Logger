#pragma once

#ifndef TINYFULL_LOGGER_H
#define TINYFULL_LOGGER_H

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <array>
#include <algorithm>
#include <chrono>
#include <type_traits>
#include <concepts>
#include <string_view>
#include <format>
#include <print>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <io.h>
    #include <windows.h>

    #ifdef ERROR
        #pragma push_macro("ERROR")
        #undef ERROR
        #define TINYFULL_LOGGER_ERROR_WAS_DEFINED
    #endif

    #ifdef min
        #pragma push_macro("min")
        #undef min
        #define TINYFULL_LOGGER_MIN_WAS_DEFINED
    #endif

    #ifdef max
        #pragma push_macro("max")
        #undef max
        #define TINYFULL_LOGGER_MAX_WAS_DEFINED
    #endif
#else
    #include <unistd.h>
#endif

namespace TinyLoggerAnsiCheck {
    inline bool checkANSI(FILE* stream) {
        if (const char* force = std::getenv("FORCE_COLOR"); force != nullptr && force[0] != '\0' && std::strcmp(force, "0") != 0) {
            return true;
        }
        if (const char* no_color = std::getenv("NO_COLOR"); no_color != nullptr && no_color[0] != '\0') {
            return false;
        }
    #if defined(_WIN32)
        int fd = _fileno(stream);
        if (!_isatty(fd)) return false;

        HANDLE hOut = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        if (hOut == INVALID_HANDLE_VALUE) return false;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return false;

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        return SetConsoleMode(hOut, dwMode) != 0;
    #else
        int fd = fileno(stream);
        if (!isatty(fd)) return false;

        const char* term = std::getenv("TERM");
        if (term == nullptr || std::strcmp(term, "dumb") == 0) {
            return false;
        }
        return true;
    #endif
    }
    inline bool supportsANSI(FILE* stream) {
        if (stream == stderr) {
            static const bool stderr_ansi = checkANSI(stderr);
            return stderr_ansi;
        }
        if (stream == stdout) {
            static const bool stdout_ansi = checkANSI(stdout);
            return stdout_ansi;
        }
        return checkANSI(stream);
    }
}

class Log {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Success,
        Warning,
        Error,
        Fatal,
        NoLevel
    };
    enum class Category {
        Action,
        Input,
        Value,
        Memory,
        Resource,
        Audio,
        Physics,
        Network,
        NoCategory
    };
    struct LogOptions {
        Level level = Level::NoLevel;
        Category category = Category::NoCategory;
        FILE* stream = nullptr;
    };
private:
    struct Style{
        std::string_view name;
        std::string_view color;
    };
    static constexpr Style getLevel(Level t) noexcept {
        switch (t) {
        case Level::Trace:   return {"TRACE",   "\x1b[38;5;244m"};
        case Level::Debug:   return {"DEBUG",   "\x1b[1;97;43m"};
        case Level::Info:    return {"INFO",    "\x1b[38;5;186m"};
        case Level::Success: return {"SUCCESS", "\x1b[32m"};
        case Level::Warning: return {"WARNING", "\x1b[33m"};
        case Level::Error:   return {"ERROR",   "\x1b[31m"};
        case Level::Fatal:   return {"FATAL",   "\x1b[1;97;41m"};
        case Level::NoLevel: return {"", ""};
        default:             return {"UNKNOWN", "\x1b[31m"};
        }
    }
    static constexpr Style getCategory(Category m) noexcept {
        switch (m) {
        case Category::Action:      return {"Action",   "\x1b[38;5;227m"};
        case Category::Input:       return {"Input",    "\x1b[94m"};
        case Category::Value:       return {"Value",    "\x1b[33m"};
        case Category::Memory:      return {"Memory",   "\x1b[38;5;118m"};
        case Category::Resource:    return {"Resource", "\x1b[38;5;98m"};
        case Category::Audio:       return {"Audio",    "\x1b[38;5;212m"};
        case Category::Physics:     return {"Physics",  "\x1b[38;5;159m"};
        case Category::Network:     return {"Network",  "\x1b[38;2;4;165;229m"};
        case Category::NoCategory:  return {"", ""};
        default:                    return {"Unknown",  "\x1b[31m"};
        }
    }
    static FILE* getDefaultStream(Level t) noexcept {
        return (t == Level::Error || t == Level::Fatal) ? stderr : stdout;
    }

    static void printHeader(FILE* stream, Level t, Category m) {
        std::array styles{getLevel(t), getCategory(m)};
        const bool not_empty = std::any_of(styles.begin(), styles.end(), [](const auto& st){
            return !st.name.empty();
        });

        if (not_empty){
            const bool color = TinyLoggerAnsiCheck::supportsANSI(stream);
            for (const auto& st : styles) {
                if (st.name.empty()) continue;
                if (color) std::print(stream, "[{}{}\x1b[0m]", st.color, st.name);
                else std::print(stream, "[{}]", st.name);
            }
            std::print(stream, " ");
        }
    }
public:
    template <typename... Args>
    static void Message(LogOptions opt, std::format_string<Args...> fmt, Args&&... args) {
        FILE* const stream = opt.stream ? opt.stream : getDefaultStream(opt.level);
        printHeader(stream, opt.level, opt.category);
        std::println(stream, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Message(std::format_string<Args...> fmt, Args&&... args) {
        Message(LogOptions{}, fmt, std::forward<Args>(args)...);
    }
};


#ifdef TINYFULL_LOGGER_ERROR_WAS_DEFINED
    #pragma pop_macro("ERROR")
    #undef TINYFULL_LOGGER_ERROR_WAS_DEFINED
#endif
#ifdef TINYFULL_LOGGER_MIN_WAS_DEFINED
    #pragma pop_macro("min")
    #undef TINYFULL_LOGGER_MIN_WAS_DEFINED
#endif
#ifdef TINYFULL_LOGGER_MAX_WAS_DEFINED
    #pragma pop_macro("max")
    #undef TINYFULL_LOGGER_MAX_WAS_DEFINED
#endif

#endif // TINYFULL_LOGGER_H