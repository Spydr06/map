#pragma once

#include <istream>
#include <optional>
#include <memory>
#include <cmath>

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

// Mercator projection utility functions

// returns distance in meters
double measure_latlon_dist(glm::vec2 from, glm::vec2 to);

static inline float rad_to_deg(float rad) {
    return rad * (180.0f / M_PI);
}

static inline glm::vec2 rad_to_deg(glm::vec2 rad) {
    return rad * glm::vec2(180.0f / M_PI);
}

static inline float deg_to_rad(float deg) {
    return deg / (180.0f / M_PI);
}

static inline glm::vec2 deg_to_rad(glm::vec2 deg) {
    return deg / glm::vec2(180.0f / M_PI);
}

static inline glm::vec2 map_project(glm::vec2 latlon) {
    return glm::vec2(
        latlon.x,
        rad_to_deg(std::log(std::tan(deg_to_rad(latlon.y) / 2 + M_PI / 4)))
    );
}

static inline glm::vec2 project_back(glm::vec2 mapped) {
    return glm::vec2(
        mapped.x,
        rad_to_deg(std::atan(std::exp(deg_to_rad(mapped.y))) * 2 - M_PI / 2)
    );
}

static inline double measure_mapped_dist(glm::vec2 from, glm::vec2 to) {
    return measure_latlon_dist(
        project_back(from),
        project_back(to)
    );
}

