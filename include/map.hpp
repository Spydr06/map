#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>

struct Node {
    typedef uint64_t Id;
    glm::vec2 coord;
};

class Way {
public:
    typedef uint64_t Id;

    Way() : m_nodes()
    {}

    ~Way() {
        if(m_vao)
            glDeleteVertexArrays(1, &m_vao);
        if(m_vbo)
            glDeleteVertexArrays(1, &m_vbo);
    }

    void create_buffers();

    void draw_buffers();

    inline void add_node(Node node) {
        increase_bbox(node.coord);
        m_nodes.push_back(node);
    }
    
    inline auto get_minmax_coord() {
        return std::make_pair(m_min_coord, m_max_coord);
    }

    inline auto min_coord() const {
        return m_min_coord;
    }

    inline auto max_coord() const {
        return m_max_coord;
    }

private:
    inline void increase_bbox(glm::vec2& coord) {
        m_min_coord.x = std::min(m_min_coord.x, coord.x);
        m_min_coord.y = std::min(m_min_coord.y, coord.y);
        m_max_coord.x = std::max(m_max_coord.x, coord.x);
        m_max_coord.y = std::max(m_max_coord.y, coord.y);
    }

    std::vector<Node> m_nodes;
    glm::vec2 m_min_coord, m_max_coord;

    GLuint m_vao = 0, m_vbo = 0;
};

class Map {
public:
    Map() : m_ways() {
    }

    inline void add_way(Way::Id id, std::unique_ptr<Way> way) {
        m_ways[id] = std::move(way);
    }
    
    inline void set_minmax_coord(std::pair<glm::vec2, glm::vec2> minmax) {
        auto [min, max] = minmax;
        m_min_coord = min;
        m_max_coord = max;
    }

    inline auto get_minmax_coord() {
        return std::make_pair(m_min_coord, m_max_coord);
    }

    inline auto min_coord() const {
        return m_min_coord;
    }

    inline auto max_coord() const {
        return m_max_coord;
    }
    
    inline auto viewport_size() const {
        return m_max_coord - m_min_coord;
    }

    inline auto& get_ways() {
        return m_ways;
    }
    
    std::unordered_map<Way::Id, std::unique_ptr<Way>> m_ways;

private:
    glm::vec2 m_min_coord, m_max_coord;
};


