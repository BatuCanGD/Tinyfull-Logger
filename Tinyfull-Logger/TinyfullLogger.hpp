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
#include <fstream>
#include <string>
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

class tlg {
public:
    enum class PrinTy {
        Terminal,
        File,
        Both
    };
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
        PrinTy printy = PrinTy::Terminal;
        FILE* stream = nullptr;
    };
private:
    struct Style{
        std::string_view name;
        std::string_view color;
    };
    struct HeadUp {
        std::string terminal;
        std::string file;
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

    static constexpr const char* log_file_name = "tinyful-logger-logs.txt";
    static void saveToFile(std::string_view msg = "", bool newline = false){
        static std::ofstream outFile(log_file_name, std::ios::app);
        if (outFile.is_open()){
            outFile << msg;
            if (newline) outFile << '\n';
        }else{
            tlg::Error("Failed to write to file!");
        }
    }

    static HeadUp getHeader(FILE* stream,LogOptions opt) {
        HeadUp hup{};

        const std::string time = std::format("[{:%H:%M:%S}]", std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())});
        hup.terminal.append(time);
        hup.file.append(time);

        std::array styles{getLevel(opt.level), getCategory(opt.category)};
        const bool not_empty = std::any_of(styles.begin(), styles.end(), [](const auto& st){
            return !st.name.empty();
        });

        if (not_empty){
            const bool color = stream ? TinyLoggerAnsiCheck::supportsANSI(stream) : false;
            for (const auto& st : styles) {
                if (st.name.empty()) continue;

                hup.file.append(std::format("[{}]", st.name));

                if (color) {
                    hup.terminal.append(std::format("[{}{}\x1b[0m]", st.color, st.name));
                }else {
                    hup.terminal.append(std::format("[{}]", st.name));
                }
            }
        }
        hup.terminal.append(" ");
        hup.file.append(" ");
        return hup;
    }
    static void SendMessage(FILE* stream, LogOptions opt, std::string_view msg){
        const auto header = getHeader(stream, opt);
        const bool writes = (opt.printy == PrinTy::File     || opt.printy == PrinTy::Both);
        const bool prints = (opt.printy == PrinTy::Terminal || opt.printy == PrinTy::Both);

        if (prints){
            std::print(stream,"{}", header.terminal);
            std::println(stream,"{}", msg);
        }
        if (writes){
            saveToFile(header.file);
            saveToFile(msg, true);
        }
    }
public:
    template <typename... Args>
    static void Message(LogOptions opt, std::format_string<Args...> fmt, Args&&... args) {
        FILE* const stream = opt.stream ? opt.stream : getDefaultStream(opt.level);
        SendMessage(stream, opt, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void Info(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Info}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Fatal(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Fatal}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Error(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Error}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Warning(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Warning}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Debug(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Debug}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Trace(std::format_string<Args...> fmt, Args&&... args){
        Message({.level = Level::Trace}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void WFile(std::format_string<Args...> fmt, Args&&... args){
        Message({.printy = PrinTy::File}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void PFile(std::format_string<Args...> fmt, Args&&... args){
        Message({.printy = PrinTy::Both}, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void Message(std::format_string<Args...> fmt, Args&&... args) {
        Message({}, fmt, std::forward<Args>(args)...);
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