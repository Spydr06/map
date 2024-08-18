#pragma once

#include <string>

namespace mlog {
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    void init(Level log_level);
    void init_from_env(const std::string& var);

    [[gnu::format(printf, 2, 3)]]
    void logln(Level level, const char* fmt, ...);
    
    [[gnu::format(printf, 2, 3)]]
    void log(Level level, const char* fmt, ...);
}

