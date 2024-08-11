#pragma once

#include "viewport.hpp"

#include <vector>

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
    
private:
    std::vector<Node> m_nodes;

    GLuint m_vao = 0, m_vbo = 0;
};

