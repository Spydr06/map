#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>

#include <glm/vec2.hpp>

struct Node {
    typedef uint64_t Id;
    glm::vec2 coord;
};

class Map {
public:
    Map() : m_nodes({}), m_min_coord(std::numeric_limits<float>::infinity()), m_max_coord(-std::numeric_limits<float>::infinity()) {
    } 

    inline void add_node(Node::Id id, Node node) {
        increase_bbox(node.coord);
        this->m_nodes.insert({id, node});
    }

    inline auto min_coord() const {
        return m_min_coord;
    }

    inline auto max_coord() const {
        return m_max_coord;
    }

    inline auto& get_nodes() {
        return m_nodes;
    }

    inline auto viewport_size() const {
        return m_max_coord - m_min_coord;
    }

private:
    inline void increase_bbox(glm::vec2& coord) {
        m_min_coord.x = std::min(m_min_coord.x, coord.x);
        m_min_coord.y = std::min(m_min_coord.y, coord.y);
        m_max_coord.x = std::max(m_max_coord.x, coord.x);
        m_max_coord.y = std::max(m_max_coord.y, coord.y);
    }

    std::unordered_map<Node::Id, Node> m_nodes;
    glm::vec2 m_min_coord, m_max_coord;
};


