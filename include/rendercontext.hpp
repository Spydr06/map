#pragma once

#include "map.hpp"
#include "inputstate.hpp"
#include "viewport.hpp"

#include <istream>
#include <memory>
#include <optional>
#include <string>

#include <GL/glew.h>

class Shader;

class RenderContext {
public:
    RenderContext(std::shared_ptr<Map> map, glm::vec2 window_size)
        : m_map(map), m_map_shader(nullptr), m_viewport(map->get_minmax_coord()), m_input_state(window_size)
    {
        init();
    }

    void draw_scene();
    void draw_ui();

    inline auto& get_viewport() {
        return m_viewport;
    }

    inline auto& get_input_state() {
        return m_input_state;
    }

private:
    void init();
    void draw_debug_info();

    std::shared_ptr<Map> m_map;
    std::unique_ptr<Shader> m_map_shader;
    
    Viewport m_viewport;
    InputState m_input_state;

    int m_bvh_max_depth;
};

class Shader {
public:
    Shader(std::istream& vertex, std::istream& fragment);
    ~Shader();
    
    bool has_error() const {
        return m_err.has_value();
    }

    const std::optional<std::string>& get_error() const {
        return m_err;
    }

    inline void use() {
        glUseProgram(m_id);
    }

    inline int id() const {
        return m_id;
    }

private:
    std::optional<std::string> m_err;
    int m_id;
};

