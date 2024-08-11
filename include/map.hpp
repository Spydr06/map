#pragma once

#include "bbox.hpp"
#include "viewport.hpp"

#include <vector>
#include <unordered_map>
#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>

struct Node {
    typedef uint64_t Id;
    glm::vec2 coord;
};

class Way : public BBox {
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
    
    inline bool in_viewport(Viewport& viewport) const {
        return m_min_coord.x < viewport.max_view().x && m_max_coord.x > viewport.min_view().x &&
            m_min_coord.y < viewport.max_view().y && m_max_coord.y > viewport.min_view().y;
    }

private:

    std::vector<Node> m_nodes;

    GLuint m_vao = 0, m_vbo = 0;
};

class Map : public BBox {
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

    inline auto& get_ways() {
        return m_ways;
    }
    
private:
    std::unordered_map<Way::Id, std::unique_ptr<Way>> m_ways;
};

