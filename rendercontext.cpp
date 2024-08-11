#include "rendercontext.hpp"
#include "imgui.h"

#include <GL/glew.h>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <iostream>

void RenderContext::init() {
    auto vertex_source = std::ifstream("shaders/map_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/map_fragment.glsl");
    if(vertex_source.bad()) {
        std::cerr << "Shader error: Shader file not found" << std::endl;
        std::exit(1);
    }

    m_map_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_map_shader->get_error()) {
        std::cerr << "Shader error:" << std::endl << *err << std::endl;
        std::exit(1);
    }

    for(auto& [_, way] : m_map->get_ways()) {
        way->create_buffers(); 
    }
}

void RenderContext::draw_debug_info() {
    ImGui::Begin("Debug info");

    auto [min, max] = m_map->get_minmax_coord();
    ImGui::Text("coordinate system: (%f, %f) to (%f, %f)", min.x, min.y, max.x, max.y);

    auto viewport = m_viewport.viewport_size();
    ImGui::Text("viewport size: (%f %f)", viewport.x, viewport.y);

    auto translation = m_viewport.get_translation();
    ImGui::Text("translation: (%f %f)", translation.x, translation.y);

    auto scale = m_viewport.get_scale(m_input_state.window_size);
    ImGui::Text("scale: (%f %f)", scale.x, scale.y);

    ImGui::Separator();

    ImGui::Text("num ways: %zu\n", m_map->get_ways().size());
    ImGui::Text("rendered: %zu\n", m_num_rendered);

    ImGui::End();
}

void RenderContext::draw() {
    m_map_shader->use();

    m_viewport.upload_uniforms(m_map_shader->id(), m_input_state.window_size);

    m_num_rendered = 0;
    for(auto& [_, way] : m_map->get_ways()) {
        if(way->in_viewport(m_viewport)) {
            way->draw_buffers();
            m_num_rendered++;
        }
    }
}

void Viewport::upload_uniforms(GLuint shader_id, glm::vec2 window_size) {
    auto scale = get_scale(window_size);
    glUniform2f(glGetUniformLocation(shader_id, "u_Scale"), scale.x, scale.y);
    glUniform2f(glGetUniformLocation(shader_id, "u_Translation"), m_translation.x, m_translation.y);
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
