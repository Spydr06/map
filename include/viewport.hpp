#pragma once

#include <utility>
#include <algorithm>

#include <GL/glew.h>
#include <glm/vec2.hpp>

#include "bbox.hpp"

class Viewport {
public:
    Viewport(std::pair<glm::vec2, glm::vec2> minmax_coords) 
        : m_min_coord(minmax_coords.first), m_max_coord(minmax_coords.second), m_min_view(m_min_coord), m_max_view(m_max_coord)
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

        auto window_aspect_ratio = glm::vec2(1.0, window_size.x / window_size.y);
        auto scale = glm::vec2(std::max(pre_scale.x, pre_scale.y)) * window_aspect_ratio * glm::vec2(m_scale_factor);

        m_min_view = -m_translation - glm::vec2(1.0) / scale;
        m_max_view = -m_translation + glm::vec2(1.0) / scale;

        return scale;
    }
    
    inline float& get_scale_factor() {
        return m_scale_factor;
    }

    inline auto viewport_size() -> glm::vec2 {
        return m_max_view - m_min_view;
    }

    inline auto min_view() const {
        return m_min_view;
    }

    inline auto max_view() const {
        return m_max_view;
    }

    inline BBox viewport_bbox() const {
        return BBox(m_min_view, m_max_view);
    }

private:
    glm::vec2 m_min_coord;
    glm::vec2 m_max_coord;
    glm::vec2 m_min_view;
    glm::vec2 m_max_view;

    float m_scale_factor = 1.0f;
    glm::vec2 m_translation;
};

