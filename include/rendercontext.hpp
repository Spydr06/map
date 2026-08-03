#pragma once

#include "map.hpp"
#include "inputstate.hpp"
#include "viewport.hpp"
#include "renderutil.hpp"

#include <memory>
#include <set>

#include <GL/glew.h>
#include <glm/vec3.hpp>

// Blue Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(1.0f, 0.953f, 0.914f)

// Red Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(1.,0.976,0.925)

// Purple Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(0.094,0.067,0.176)

// Green Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(0.082,0.075,0.078)
#define DEFAULT_CLEAR_COLOR glm::vec3(0.184,0.243,0.275)

// Grayscale Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(0.0, 0.0, 0.0)

class RenderContext {
public:
    RenderContext(glm::vec2 window_size)
        : m_elements(), m_viewport(), m_input_state(window_size)
    {}

    void draw_scene();
    void draw_ui();

    void remove_elements();

    inline void add_element(std::shared_ptr<RenderElement> element) {
        m_elements.insert(element);
    }

    template<std::derived_from<RenderElement> T>
    inline std::shared_ptr<T> get_element() {
        for(const auto& element : m_elements) {
            if(auto found = std::dynamic_pointer_cast<T>(element))
                return found; 
        }
        return nullptr;
    }

    inline void remove_element(std::shared_ptr<RenderElement> element) {
        m_elements.erase(element);
    }

    inline auto& get_viewport() {
        return m_viewport;
    }

    inline auto& get_input_state() {
        return m_input_state;
    }

    inline auto& get_clear_color() {
        return m_clearcolor;
    }

private:
    using element_iter = std::multiset<std::shared_ptr<RenderElement>>::iterator;
    element_iter get_first_element();

    void draw_debug_info();

    std::multiset<std::shared_ptr<RenderElement>, RenderElement::Comparator> m_elements;
    
    glm::vec3 m_clearcolor = DEFAULT_CLEAR_COLOR;
    Viewport m_viewport;
    InputState m_input_state;

    bool m_disable_fill = false;
};

