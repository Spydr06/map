#include "rendercontext.hpp"
#include "imgui.h"
#include "renderutil.hpp"

#include <GL/glew.h>

#include <cassert>
#include <sstream>
#include <iostream>

void RenderContext::draw_debug_info() {
    ImGui::Begin("Debug info");

    auto viewport = m_viewport.viewport_size();
    ImGui::Text("viewport size: (%f %f)", viewport.x, viewport.y);

    auto view_min = m_viewport.min_view();
    auto view_max = m_viewport.max_view();

    auto tl = view_min;
    auto tr = glm::vec2(view_max.x, view_min.y);
    auto bl = glm::vec2(view_min.x, view_max.x);

    auto view_width_m = measure_mapped_dist(tl, tr);
    auto view_width_h = measure_mapped_dist(tl, bl);

    ImGui::Text("viewport size (meters): (%f %f)", view_width_m, view_width_h);
    
    auto translation = m_viewport.get_translation();
    ImGui::Text("translation: (%f %f)", translation.x, translation.y);

    auto scale = m_viewport.get_scale(m_input_state.window_size);
    ImGui::Text("scale: (%f, %f) | x%f", scale.x, scale.y, m_viewport.get_zoom_factor());

    ImGui::Separator();

    ImGui::Text("raw cursor pos: (%f %f)", m_input_state.last_cursor_pos.x, m_input_state.last_cursor_pos.y);
    ImGui::Text("mapped cursor pos: (%f %f)", m_input_state.mapped_cursor_pos.x, m_input_state.mapped_cursor_pos.y);

    ImGui::SeparatorText("rendering options");

    ImGui::Checkbox("show mesh", &m_disable_fill);
    ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&m_clearcolor));

    ImGui::End();
}

void RenderContext::draw_ui() {
    ImGui::BeginMainMenuBar();

    auto first = get_first_element();
    for(auto it = first; it != m_elements.end(); it++) {
        (*it)->menu_item();
    }

    ImGui::EndMainMenuBar();

    draw_debug_info();

    for(auto it = first; it != m_elements.end(); it++) {
        (*it)->draw_ui(m_input_state);
    }
}

RenderContext::element_iter RenderContext::get_first_element() {
    element_iter first = m_elements.begin();
    for(auto it = m_elements.rbegin(); it != m_elements.rend(); it++) {
        if(!(*it)->translucent()) {
            first = std::prev(it.base());
            break;
        }
    }

    return first;
}

void RenderContext::draw_scene() {
    glPolygonMode(GL_FRONT_AND_BACK, m_disable_fill ? GL_LINE : GL_FILL);

    for(auto it = get_first_element(); it != m_elements.end(); it++) {
        (*it)->draw_scene(m_viewport, m_input_state);
    }
}

void RenderContext::remove_elements() {
    std::erase_if(m_elements, [](const auto& element) {
        return element->remove();
    });
}

void Viewport::upload_uniforms(const Shader& shader, glm::vec2 window_size) {
    auto scale = get_scale(window_size);
    shader.upload_uniform("u_Scale", scale);
    shader.upload_uniform("u_Translation", m_translation);
}

static int load_shader(GLenum type, std::istream& input, std::optional<std::string>& err) {
    int id = glCreateShader(type);
    if(!id)
        return 0;

    std::ostringstream strbuf;
    strbuf << input.rdbuf();
    std::string str = strbuf.str();

    const char* ptr = str.c_str();
    const GLint buffer_size = str.size();

    glShaderSource(id, 1, static_cast<const GLchar* const*>(&ptr), &buffer_size);

    glCompileShader(id);

    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(success == GL_TRUE)
        return id;

    GLint log_length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);

    std::string log;
    log.reserve(log_length);
    log.resize(log_length);

    GLint written;
    glGetShaderInfoLog(id, log_length, &written, &log[0]);

    err = log;

    return 0;
}

Shader::Shader(std::istream& vertex_input, std::istream& fragment_input) {
    if(!(m_id = glCreateProgram())) {
        assert(false);
    }

    int vertex = load_shader(GL_VERTEX_SHADER, vertex_input, m_err);
    int fragment = load_shader(GL_FRAGMENT_SHADER, fragment_input, m_err);
    if(!fragment || !vertex)
        return;

    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);

    glLinkProgram(m_id);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
}

Shader::~Shader() {
    if(!has_error())
        glDeleteProgram(m_id);
}

