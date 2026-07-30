#include "log.hpp"
#include "main.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <unordered_map>

#include <glm/vec4.hpp>
#include <imgui.h>

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
        std::string ansi_color;
        ImVec4 console_color;
    } log_level_table[4] = {
        {"debug", "\033[0m",  ImVec4(0.7, 0.7, 0.7, 1.0) },
        {"info",  "\033[36m", ImVec4(1.0, 1.0, 1.0, 1.0) },
        {"warn",  "\033[33m", ImVec4(1.0, 1.0, 0.0, 1.0) },
        {"error", "\033[31m", ImVec4(1.0, 0.0, 0.0, 1.0) },
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
            std::printf("%s\033[1m", log_level.ansi_color.c_str());

        std::printf("[ %s ] ", log_level.str.c_str());    

        if(emit_colors)
            std::printf("\033[22m");
    }

    void logln(Level level, const char* fmt, ...) {
        std::va_list args;
        va_start(args, fmt);

        std::va_list args_copy;
        va_copy(args_copy, args);

        size_t size = std::vsnprintf(nullptr, 0, fmt, args);
        std::string line(size + 1, '\0');
        std::vsprintf(&line[0], fmt, args_copy);

        if(level >= min_log_level) {
            print_log_fmt(level, &fmt);
            std::puts(line.c_str());
        }

        va_end(args_copy);
        va_end(args);

        emit_newline = false;

        if(auto c = Console::get())
            c->push_line(level, line);
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

std::shared_ptr<Console> Console::get() {
    if(!context)
        return nullptr;
    return context->get_element<Console>();
}

void Console::draw_scene(Viewport& viewport, InputState& input) {}

void Console::draw_ui(InputState& input) {
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);

    ImGui::BeginMenuBar();

    if(ImGui::BeginMenu("Scroll")) {
        if(ImGui::MenuItem("Auto Scroll", nullptr, m_auto_scroll, true))
            m_auto_scroll = !m_auto_scroll;

        if(ImGui::MenuItem("Scroll Down", nullptr, false, true))
            m_scroll_down = true;

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Filter")) {
        for(size_t i = 0; i < m_filter.size(); i++) {
            const auto& log_level = mlog::log_level_table[i];

            if(ImGui::MenuItem(log_level.str.c_str(), nullptr, m_filter[i], true))
                m_filter[i] = !m_filter[i];
        }
        
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();

    if(ImGui::BeginChild("ScrollRegion##")) {
        ImGui::PushTextWrapPos();

        for(size_t i = m_back; i != m_front; i = (i + 1) % m_capacity) {
            const auto& line = m_lines[i];
            const auto& log_level = mlog::log_level_table[static_cast<int>(line.level)];

            if(!m_filter[static_cast<int>(line.level)])
                continue;

            ImGui::PushStyleColor(ImGuiCol_Text, log_level.console_color);
            ImGui::Text("[ %s ] ", log_level.str.c_str());    
            ImGui::SameLine();
            ImGui::TextUnformatted(line.line.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::PopTextWrapPos();

        if(m_scroll_down) {
            ImGui::SetScrollHereY(1.0f);
            m_scroll_down = false;
        }
    }

    ImGui::EndChild();

    ImGui::End();
}

void Console::push_line(mlog::Level level, const std::string& line) {
    if((m_front + 1) % m_capacity == m_back) {
        m_back = (m_back + 1) % m_capacity;
    }

    m_lines[m_front] = LogLine{level, line};
    m_front = (m_front + 1) % m_capacity;

    m_scroll_down = m_auto_scroll;
}

