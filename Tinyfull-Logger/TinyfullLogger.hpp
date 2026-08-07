#pragma once
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string_view>
#include <format>
#include <print>

#if defined(_WIN32)
    #include <io.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif


namespace {
    bool checkANSI(FILE* stream) {
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

    bool supportsANSI(FILE* stream) {
        if (stream == stderr) {
            static const bool stderr_ansi = checkANSI(stderr);
            return stderr_ansi;
        }
        static const bool stdout_ansi = checkANSI(stdout);
        return stdout_ansi;
    }
}

class Log {
public:
    enum class Level {
        Success, // action happened/completed correctly/successfully
        Info,    // normal message, informative
        Warning, // something might have not have been changed but everything else is fine
        Error,   // something broke or didnt trigger but it didnt crash the game
        Fatal    // something extremely wrong happened and it could have crashed the program
    };
    enum class Category {
        Input, // User Input
        Value, // Value Changes
        Memory,// Pointers
        Action // User Action
    };
private:
    static constexpr const char* getMessageLevel(Level t, bool use_color) {
        if (!use_color) {
            switch (t) {
            case Level::Success: return "SUCCESS";
            case Level::Info:    return "INFO";
            case Level::Warning: return "WARNING";
            case Level::Error:   return "ERROR";
            case Level::Fatal:   return "FATAL";
            default: return "LEVEL FETCH FAILURE";
            }
        }
        switch (t) {
        case Level::Success: return "\033[32mSUCCESS\033[0m";
        case Level::Info:    return "\033[38;2;208;200;140mINFO\033[0m";
        case Level::Warning: return "\033[33mWARNING\033[0m";
        case Level::Error:   return "\033[31mERROR\033[0m";
        case Level::Fatal:   return "\033[1;97;41mFATAL\033[0m";
        default: return "\033[31mLEVEL FETCH FAILURE\033[0m";
        }
    }

    static constexpr const char* getMessageCategory(Category m, bool use_color) {
        if (!use_color) {
            switch (m) {
            case Category::Input:  return "Input";
            case Category::Value:  return "Value";
            case Category::Action: return "Action";
            case Category::Memory: return "Memory";
            default: return "Category fetch failure";
            }
        }
        switch (m) {
        case Category::Input:  return "\033[94mInput\033[0m";
        case Category::Value:  return "\033[33mValue\033[0m";
        case Category::Action: return "\033[35mAction\033[0m";
        case Category::Memory: return "\033[32mMemory\033[0m";
        default: return "\033[31mCategory fetch failure\033[0m";
        }
    }
public:
    static void Message(Level t, Category m, std::string_view details = "") {
        FILE* stream = (t == Level::Error || t == Level::Fatal) ? stderr : stdout;
        bool color_enabled = ::supportsANSI(stream);
        std::println(stream, "[{}][{}] {}", getMessageLevel(t, color_enabled), getMessageCategory(m, color_enabled), details);
    }
    template <typename... Args>
    static void Message(Level t, Category m, std::format_string<Args...> fmt, Args&&... args) {
        FILE* stream = (t == Level::Error || t == Level::Fatal) ? stderr : stdout;
        bool color_enabled = ::supportsANSI(stream);
        std::println(stream, "[{}][{}] {}", getMessageLevel(t, color_enabled), getMessageCategory(m, color_enabled), std::format(fmt, std::forward<Args>(args)...));
    }
};
