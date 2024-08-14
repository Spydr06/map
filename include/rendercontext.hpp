#pragma once

#include "map.hpp"
#include "inputstate.hpp"
#include "viewport.hpp"
#include "renderutil.hpp"

#include <memory>

#include <GL/glew.h>

class RenderContext {
public:
    RenderContext(std::shared_ptr<Map> map, glm::vec2 window_size)
        : m_map(map), m_elements({map}), m_viewport(map->get_minmax_coord()), m_input_state(window_size)
    {}

    void draw_scene();
    void draw_ui();

    inline auto& get_viewport() {
        return m_viewport;
    }

    inline auto& get_input_state() {
        return m_input_state;
    }

private:
    void draw_debug_info();

    std::shared_ptr<Map> m_map;
    std::vector<std::shared_ptr<RenderElement>> m_elements;
    
    Viewport m_viewport;
    InputState m_input_state;

};

