#pragma once

#include "renderutil.hpp"
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

class Console : public RenderElement {
public:
    struct LogLine {
        mlog::Level level;
        std::string line;
    };

    static constexpr size_t MAX_LINES = 256;

    static std::shared_ptr<Console> get();

    Console(size_t max_lines = MAX_LINES)
        : m_capacity(max_lines)
    {
        m_lines.resize(m_capacity);

        push_line(mlog::INFO, "OSM Viewer and Heightmap generator");
        push_line(mlog::INFO, "Copyright (C) 2026 Spydr06; Licensed under the MIT License.");
        push_line(mlog::INFO, "This is free software; see the source for copying conditions. There is NO warranty.");
        push_line(mlog::INFO, "");
    }

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;
    void push_line(mlog::Level level, const std::string& line);

private:
    bool m_auto_scroll = true;
    bool m_scroll_down = false;

    std::array<bool, 4> m_filter{true, true, true, true};

    size_t m_capacity = 0, m_front = 0, m_back = 0;
    std::vector<LogLine> m_lines{};
};

