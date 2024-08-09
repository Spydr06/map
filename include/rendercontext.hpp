#pragma once

#include "map.hpp"

#include <istream>
#include <memory>
#include <optional>
#include <string>

#include <GL/glew.h>

class Shader;

class RenderContext {
public:
    RenderContext(std::shared_ptr<Map> map)
        : m_map(map), m_map_shader(nullptr)
    {
        init();
    }

    void draw();

private:
    void init();

    std::shared_ptr<Map> m_map;
    std::unique_ptr<Shader> m_map_shader;

    GLuint m_map_vao, m_map_vbo;
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

