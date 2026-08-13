#pragma once
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <array>
#include <string_view>
#include <format>
#include <print>

#if defined(_WIN32)
    #include <io.h>
    #include <windows.h>
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
    };
private:
    struct Style{
        std::string_view name;
        std::string_view color;
    };
    static constexpr Style getLevel(Level t) {
        switch (t) {
        case Level::Trace:   return {"TRACE",   "\x1b[91m"};
        case Level::Debug:   return {"DEBUG",   "\x1b[1;97;43m"};
        case Level::Info:    return {"INFO",    "\x1b[38;2;208;200;140m"};
        case Level::Success: return {"SUCCESS", "\x1b[32m"};
        case Level::Warning: return {"WARNING", "\x1b[33m"};
        case Level::Error:   return {"ERROR",   "\x1b[31m"};
        case Level::Fatal:   return {"FATAL",   "\x1b[1;97;41m"};
        case Level::NoLevel: return {"", ""};
        default:             return {"UNKNOWN", "\x1b[31m"};
        }
    }
    static constexpr Style getCategory(Category m) {
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
    static bool usesErrorStream(Level t){
        return t == Level::Error || t == Level::Fatal || t == Level::Trace;
    }

    static void printHeader(FILE* stream, Level t, Category m) {
        bool color = TinyLoggerAnsiCheck::supportsANSI(stream);
        std::array styles{getLevel(t), getCategory(m)};

        for (const auto& st : styles) {
            if (st.name.empty()) continue;
            if (color) std::print(stream, "[{}{}\x1b[0m]", st.color, st.name);
            else std::print(stream, "[{}]", st.name);
        }
        std::print(stream, " ");
    }
public:
    template <typename... Args>
    static void Message(LogOptions opt, std::format_string<Args...> fmt, Args&&... args) {
        FILE* stream = usesErrorStream(opt.level) ? stderr : stdout;
        printHeader(stream, opt.level, opt.category);
        std::println(stream, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Message(std::format_string<Args...> fmt, Args&&... args) {
        Message(LogOptions{}, fmt, std::forward<Args>(args)...);
    }
};
