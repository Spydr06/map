#include "map.hpp"

#include <GL/glew.h>

static_assert(sizeof(Node) == sizeof(float) * 2);

void Way::create_buffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    assert(m_vao != 0);
    assert(m_vbo != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_nodes.size() * sizeof(Node), &m_nodes[0], GL_STATIC_DRAW);

    glBindVertexArray(m_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Way::draw_buffers() {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, m_nodes.size());
}

