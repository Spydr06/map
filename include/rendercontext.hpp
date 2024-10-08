#pragma once

#include "map.hpp"
#include "inputstate.hpp"
#include "viewport.hpp"
#include "renderutil.hpp"

#include <memory>
#include <set>

#include <GL/glew.h>

class RenderContext {
public:
    RenderContext(std::shared_ptr<Map> map, glm::vec2 window_size)
        : m_map(map), m_elements({map}), m_viewport(map->get_minmax_coord()), m_input_state(window_size)
    {}

    void draw_scene();
    void draw_ui();

    inline void add_element(std::shared_ptr<RenderElement> element) {
        m_elements.insert(element);
    }

    inline auto& get_viewport() {
        return m_viewport;
    }

    inline auto& get_input_state() {
        return m_input_state;
    }

private:
    void draw_debug_info();

    std::shared_ptr<Map> m_map;
    std::multiset<std::shared_ptr<RenderElement>, RenderElement::Comparator> m_elements;
    
    Viewport m_viewport;
    InputState m_input_state;

    bool m_disable_fill = false;
};

