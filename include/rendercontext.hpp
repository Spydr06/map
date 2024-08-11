#pragma once

#include "map.hpp"
#include "inputstate.hpp"

#include <istream>
#include <memory>
#include <optional>
#include <string>

#include <GL/glew.h>

class Viewport {
public:
    Viewport(std::pair<glm::vec2, glm::vec2> minmax_coords) 
        : m_min_coord(minmax_coords.first), m_max_coord(minmax_coords.second)
    {
        default_translation();
    }

    void upload_uniforms(GLuint shader_id, glm::vec2 window_size);

    inline void move(glm::vec2 offset, glm::vec2 window_size) {
        m_translation += (offset / get_scale(window_size) / window_size * glm::vec2(2));
    }

    inline void default_translation() {
        m_translation = -m_min_coord;
    }

    inline auto& get_translation() {
        return m_translation;
    }

    inline auto get_scale(glm::vec2 window_size) -> glm::vec2 {
        auto pre_scale = glm::vec2(2.0) / (m_max_coord - m_min_coord);


        auto aspect_ratio = glm::vec2(1.0, window_size.x / window_size.y);
        return glm::vec2(std::max(pre_scale.x, pre_scale.y)) * aspect_ratio * glm::vec2(m_scale_factor);
    }
    
    inline float& get_scale_factor() {
        return m_scale_factor;
    }

    inline auto viewport_size() -> glm::vec2 {
        return m_max_coord - m_min_coord;
    }

private:
    glm::vec2 m_min_coord;
    glm::vec2 m_max_coord;

    float m_scale_factor = 1.0f;
    glm::vec2 m_translation;
};

class Shader;

class RenderContext {
public:
    RenderContext(std::shared_ptr<Map> map, glm::vec2 window_size)
        : m_map(map), m_map_shader(nullptr), m_viewport(map->get_minmax_coord()), m_input_state(window_size)
    {
        init();
    }

    void draw();
    void draw_debug_info();

    inline auto& get_viewport() {
        return m_viewport;
    }

    inline auto& get_input_state() {
        return m_input_state;
    }

private:
    void init();

    std::shared_ptr<Map> m_map;
    std::unique_ptr<Shader> m_map_shader;
    
    Viewport m_viewport;
    InputState m_input_state;
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

