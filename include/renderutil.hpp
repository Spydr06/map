#pragma once

#include <glm/ext/vector_float3.hpp>
#include <istream>
#include <optional>
#include <memory>
#include <cmath>

#include <GL/glew.h>
#include <utility>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "glm/ext/vector_int2.hpp"
#include "inputstate.hpp"
#include "viewport.hpp"

class RenderContext;

class Texture {
public:
    Texture(GLuint width, GLuint height, GLenum format = GL_RGB, GLenum data_type = GL_UNSIGNED_BYTE);
    ~Texture();

    inline void bind(GLenum slot = GL_TEXTURE_2D) const {
        glBindTexture(slot, m_id);
    }

    inline GLuint id() const {
        return m_id;
    }

    inline std::pair<GLuint, GLuint> size() {
        return std::make_pair(m_width, m_height);
    }

private:
    GLuint m_id;
    GLuint m_width, m_height;
};

class Framebuffer {
public:
    Framebuffer(GLuint width, GLuint height);
    ~Framebuffer();

    inline void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);
    }

    inline GLuint id() const {
        return m_id;
    }

    inline GLuint texture_id() const {
        return m_texture->id();
    }

    inline std::pair<GLuint, GLuint> texture_size() const {
        return m_texture->size();
    }

private:
    GLuint m_id;
    std::unique_ptr<Texture> m_texture;
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

    inline GLuint id() const {
        return m_id;
    }

    inline void upload_uniform(const std::string& uniform, bool value) const {
        glUniform1i(glGetUniformLocation(m_id, uniform.c_str()), value);
    }

    inline void upload_uniform(const std::string& uniform, float value) const {
        glUniform1f(glGetUniformLocation(m_id, uniform.c_str()), value);
    }

    inline void upload_uniform(const std::string& uniform, GLuint value) const {
        glUniform1i(glGetUniformLocation(m_id, uniform.c_str()), value);
    }

    inline void upload_uniform(const std::string& uniform, GLint value) const {
        glUniform1i(glGetUniformLocation(m_id, uniform.c_str()), value);
    }

    template<std::size_t N>
    inline void upload_uniform(const std::string& uniform, std::array<GLuint, N>& value) const {
        auto location = static_cast<std::size_t>(glGetUniformLocation(m_id, uniform.c_str()));
        assert(location + N <= GL_MAX_UNIFORM_LOCATIONS);

        glUniform1iv(location, static_cast<GLsizei>(N), reinterpret_cast<GLint*>(value.data()));
    }

    inline void upload_uniform(const std::string& uniform, glm::vec2 value) const {
        glUniform2f(glGetUniformLocation(m_id, uniform.c_str()), value.x, value.y);
    }

    inline void upload_uniform(const std::string& uniform, glm::vec3 value) const {
        glUniform3f(glGetUniformLocation(m_id, uniform.c_str()), value.x, value.y, value.z);
    }

    inline void upload_uniform(const std::string& uniform, glm::vec4 value) const {
        glUniform4f(glGetUniformLocation(m_id, uniform.c_str()), value.x, value.y, value.z, value.w);
    }

    template<std::size_t N>
    inline void upload_uniform(const std::string& uniform, std::array<glm::vec4, N>& value) const {
        auto location = static_cast<std::size_t>(glGetUniformLocation(m_id, uniform.c_str()));
        assert(location + N <= GL_MAX_UNIFORM_LOCATIONS);

        glUniform4fv(location, static_cast<GLsizei>(N), reinterpret_cast<float*>(value.data()));
    }

    inline void upload_uniform(const std::string& uniform, glm::ivec2 value) const {
        glUniform2i(glGetUniformLocation(m_id, uniform.c_str()), value.x, value.y);
    }

private:
    std::optional<std::string> m_err;
    GLuint m_id;
};

class Model {
public:
    Model(std::vector<glm::vec3> vertices, std::vector<GLuint> indices);
    ~Model();

private:
    GLuint m_vao, m_vbo, m_ebo;

    std::vector<glm::vec3> m_vertices;
    std::vector<GLuint> m_indices;
};

class CubeModel : public Model {
public:
    CubeModel(float size = 1.0f);
};

class RenderElement {
public:
    struct Comparator {
        bool operator()(const std::shared_ptr<RenderElement>& lhs, const std::shared_ptr<RenderElement>& rhs) const {
            return *lhs < *rhs;
        }
    };

    RenderElement() {}

    virtual void menu_item() {};

    virtual void draw_scene(Viewport& viewport, InputState& input) = 0;
    virtual void draw_ui(InputState& input) = 0;

    virtual void on_attach(RenderContext& context) {}

    virtual int get_z_index() const {
        return 0;
    };

    virtual bool translucent() const {
        return true;
    }

    virtual bool remove() const {
        return false;
    }

    bool operator<(const RenderElement& other) const {
        return this->get_z_index() < other.get_z_index();
    }
};

// Mercator projection utility functions

// returns distance in meters
double measure_latlon_dist(glm::vec2 from, glm::vec2 to);

static inline double rad_to_deg(double rad) {
    return rad * (180.0 / M_PI);
}

static inline glm::dvec2 rad_to_deg(glm::dvec2 rad) {
    return rad * glm::dvec2(180.0 / M_PI);
}

static inline double deg_to_rad(double deg) {
    return deg / (180.0 / M_PI);
}

static inline glm::vec2 deg_to_rad(glm::dvec2 deg) {
    return deg / glm::dvec2(180.0 / M_PI);
}

static inline glm::dvec2 map_project(glm::dvec2 latlon) {
    return glm::dvec2(
        latlon.x,
        rad_to_deg(std::log(std::tan(deg_to_rad(latlon.y) / 2 + M_PI / 4)))
    );
}

static inline glm::dvec2 project_back(glm::dvec2 mapped) {
    return glm::dvec2(
        mapped.x,
        rad_to_deg(std::atan(std::exp(deg_to_rad(mapped.y))) * 2 - M_PI / 2)
    );
}

static inline double measure_mapped_dist(glm::dvec2 from, glm::dvec2 to) {
    return measure_latlon_dist(
        project_back(from),
        project_back(to)
    );
}

static inline double cross_product_z(glm::dvec2 a, glm::dvec2 b) {
    return a.x * b.y - a.y * b.x;
}

static inline bool is_point_in_triangle(glm::dvec2 p, glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    glm::dvec2 ab = b - a, bc = c - b, ca = a - c;
    glm::dvec2 ap = p - a, bp = p - b, cp = p - c;

    return cross_product_z(ab, ap) <= 0.0f && cross_product_z(bc, bp) <= 0.0f && cross_product_z(ca, cp) <= 0.0f;
}

