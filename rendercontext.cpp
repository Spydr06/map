#include "rendercontext.hpp"

#include <GL/glew.h>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

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

    std::vector<float> vertices{};
    vertices.reserve(m_map->get_nodes().size() * 2);
    
    for(auto& node : m_map->get_nodes()) {
        vertices.push_back(node.second.coord.x);
        vertices.push_back(node.second.coord.y);
    }

    std::cout << vertices.size() << " points" << std::endl;
    std::cout << m_map->min_coord().x << " " << m_map->min_coord().y << " to " << m_map->max_coord().x << " " << m_map->max_coord().y << std::endl;
    std::cout << m_map->viewport_size().x << " " << m_map->viewport_size().y << std::endl;

    glGenBuffers(1, &m_map_vbo);
    assert(m_map_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_map_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_map_vao);
    assert(m_map_vao != 0);

    glBindVertexArray(m_map_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 2, 0);
}

void RenderContext::draw() {
    m_map_shader->use();

    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    int min_viewport_loc = glGetUniformLocation(m_map_shader->id(), "u_MinViewport");
    glUniform2f(min_viewport_loc, m_map->min_coord().x, m_map->min_coord().y);
    
    int max_viewport_loc = glGetUniformLocation(m_map_shader->id(), "u_MaxViewport");
    glUniform2f(max_viewport_loc, m_map->max_coord().x, m_map->max_coord().x);

    glBindVertexArray(m_map_vao);
    glBindBuffer(m_map_vbo, GL_ARRAY_BUFFER);
    glDrawArrays(GL_POINTS, 0, m_map->get_nodes().size() * 2);
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
