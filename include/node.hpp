#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include "way.hpp"

struct Node {
    typedef std::uint64_t Id;

    Node(Id id, glm::vec2 coord) 
        : m_id(id), m_coord(coord)
    {}

    inline WayPoint as_waypoint() const {
        return WayPoint(m_coord);
    }

    Id m_id;
    glm::vec2 m_coord;
    std::unique_ptr<std::unordered_map<std::string, std::string>> m_tags = nullptr;
};
