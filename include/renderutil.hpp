#pragma once

#include <istream>
#include <optional>
#include <memory>

#include <GL/glew.h>

#include "inputstate.hpp"
#include "viewport.hpp"

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

class RenderElement {
public:
    struct Comparator {
        bool operator()(const std::shared_ptr<RenderElement>& lhs, const std::shared_ptr<RenderElement>& rhs) const {
            return *lhs < *rhs;
        }
    };

    RenderElement() {}

    virtual void draw_scene(Viewport& viewport, InputState& input) = 0;
    virtual void draw_ui(InputState& input) = 0;

    virtual int get_z_index() const {
        return 0;
    };

    bool operator<(const RenderElement& other) const {
        return this->get_z_index() < other.get_z_index();
    }
};

