#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <unordered_map>

namespace mlog {
    static Level min_log_level = Level::INFO;
    static bool emit_colors = true;
    static bool emit_newline = false;

    static const std::unordered_map<std::string, Level> log_level_env_table({
        {"DEBUG", Level::DEBUG},
        {"INFO", Level::INFO},
        {"WARN", Level::WARN},
        {"ERROR", Level::ERROR}
    });

    static const struct {
        std::string str;
        std::string color;
    } log_level_table[4] = {
        {"debug", "\033[0m"},
        {"info", "\033[36m"},
        {"warn", "\033[33m"},
        {"error", "\033[31m"}
    };

    static const std::string color_reset = "\033[0m";

    void init(Level log_level) {
        min_log_level = log_level;
        emit_colors = true;
    }

    void init_from_env(const std::string& var) {
        const char* val = std::getenv(var.c_str());
        if(!val) {
            init(min_log_level);
            return;
        }

        auto log_level = log_level_env_table.find(val);
        if(log_level == log_level_env_table.end()) {
            logln(Level::ERROR, "Unrecognized `%s` value `%s`, expect one of [DEBUG,INFO,WARN,ERROR]", var.c_str(), val);
            init(min_log_level);
            return;
        }
        
        init(log_level->second);
    }

    void print_log_fmt(Level level, const char** fmt) {
        if(emit_newline && (*fmt)[0] != '\r')
            std::putc('\n', stdout);

        while(emit_newline && (*fmt)[0] == '\r') {
            std::putc('\r', stdout);
            (*fmt)++;
        }

        auto& log_level = log_level_table[static_cast<int>(level)];

        if(emit_colors)
            std::printf("%s\033[1m", log_level.color.c_str());

        std::printf("[ %s ] ", log_level.str.c_str());    

        if(emit_colors)
            std::printf("\033[22m");
    }

    void logln(Level level, const char* fmt, ...) {
        if(level < min_log_level)
            return;

        print_log_fmt(level, &fmt);

        std::va_list args;
        va_start(args, fmt);

        std::vfprintf(stdout, fmt, args);
        if(emit_colors)
            std::printf("%s", color_reset.c_str());
        std::putc('\n', stdout);

        va_end(args);

        emit_newline = false;
    }

    void log(Level level, const char* fmt, ...) {
        if(level < min_log_level)
            return;

        print_log_fmt(level, &fmt);
        
        std::va_list args;
        va_start(args, fmt);

        std::vfprintf(stdout, fmt, args);
        if(emit_colors)
            std::printf("%s", color_reset.c_str());
        std::fflush(stdout);
        
        va_end(args);

        emit_newline = fmt[std::strlen(fmt) - 1] != '\n';
    }
}

