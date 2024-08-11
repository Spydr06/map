#pragma once

#include <utility>
#include <algorithm>

#include <glm/vec2.hpp>

class BBox {
public:
    BBox() 
        : m_min_coord(std::numeric_limits<float>::infinity()), m_max_coord(-std::numeric_limits<float>::infinity())
    {}

    BBox(std::pair<glm::vec2, glm::vec2> minmax_coords)
        : m_min_coord(minmax_coords.first), m_max_coord(minmax_coords.second)
    {}

    BBox(glm::vec2 min_coord, glm::vec2 max_coord)
        : m_min_coord(min_coord), m_max_coord(max_coord)
    {}

    inline void set_minmax_coord(std::pair<glm::vec2, glm::vec2> minmax_coords) {
        m_min_coord = minmax_coords.first;
        m_max_coord = minmax_coords.second;
    }

    inline auto get_minmax_coord() const {
        return std::make_pair(m_min_coord, m_max_coord);
    }

    inline auto min_coord() const {
        return m_min_coord;
    }

    inline auto max_coord() const {
        return m_max_coord;
    }

    inline auto bbox_size() const {
        return m_max_coord - m_min_coord;
    }

    inline bool intersects(BBox& other) {
        return m_min_coord.x < other.m_max_coord.x && m_max_coord.x > other.m_min_coord.x &&
            m_min_coord.y < other.m_max_coord.y && m_max_coord.y > other.m_min_coord.y;
    }

protected:
    inline void increase_bbox(glm::vec2& coord) {
        m_min_coord.x = std::min(m_min_coord.x, coord.x);
        m_min_coord.y = std::min(m_min_coord.y, coord.y);
        m_max_coord.x = std::max(m_max_coord.x, coord.x);
        m_max_coord.y = std::max(m_max_coord.y, coord.y);
    }

    glm::vec2 m_min_coord, m_max_coord;
};

