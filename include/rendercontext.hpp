#pragma once

#include "map.hpp"
#include "viewport.hpp"
#include "renderutil.hpp"
#include "inputstate.hpp"

#include <memory>
#include <optional>
#include <set>

#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <GLFW/glfw3.h>

// Red Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(1.,0.976,0.925)

// Purple Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(0.094,0.067,0.176)

// Green Theme:
// #define DEFAULT_CLEAR_COLOR glm::vec3(0.082,0.075,0.078)

class LoaderContext {
public:
    LoaderContext(GLFWwindow* window)
        : m_window(window)
    {}

    LoaderContext(const LoaderContext&) = delete;
    ~LoaderContext() {
        glfwDestroyWindow(m_window);
        mlog::logln(mlog::DEBUG, "OpenGL child context deleted");
    }

    bool make_current();
    void finalize();

private:
    GLFWwindow* m_window;
};


class RenderContext {
public:
    RenderContext(GLFWwindow* window, glm::vec2 window_size)
        : m_elements(), m_viewport(), m_input_state(window_size), m_window(window)
    {}

    void draw_scene();
    void draw_ui();

    void remove_elements();

    std::optional<std::unique_ptr<LoaderContext>> create_loader_context();

    void add_map(std::shared_ptr<Map> map);

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
        if(auto view = get_element<MapView>()) {
            if(view->get_theme() != nullptr)
                return view->get_theme()->background();
        }
        return m_clearcolor;
    }

private:
    using element_iter = std::multiset<std::shared_ptr<RenderElement>>::iterator;
    element_iter get_first_element();

    void draw_debug_info();

    std::multiset<std::shared_ptr<RenderElement>, RenderElement::Comparator> m_elements;
    
    glm::vec3 m_clearcolor = glm::vec3(0.0);
    Viewport m_viewport;
    InputState m_input_state;
    GLFWwindow *m_window;

    bool m_disable_fill = false;
};

