#include "bvh.hpp"

#include <limits>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

BVH::BVH(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth, size_t depth)
    : BBox(minmax_coords), m_children(std::make_pair(nullptr, nullptr)), m_ways()
{
    if(depth + 1 >= max_depth)
        return;

    glm::vec2 size = bbox_size();
    
    if(size.x > size.y) {
        // split vertical

        //glm::vec2 split(m_min_coord.x + size.x / 2.0f, m_min_coord.y);
        float split_x = m_min_coord.x + size.x / 2.0f;
        auto& [ left, right ] = m_children;

        left = std::make_unique<BVH>(std::make_pair(m_min_coord, glm::vec2(split_x, m_max_coord.y)), max_depth, depth + 1);
        right = std::make_unique<BVH>(std::make_pair(glm::vec2(split_x, m_min_coord.y), m_max_coord), max_depth, depth + 1);
    }
    else {
        // split horizontal

        float split_y = m_min_coord.y + size.y / 2.0f;
        auto& [ top, bottom ] = m_children;

        top = std::make_unique<BVH>(std::make_pair(m_min_coord, glm::vec2(m_max_coord.x, split_y)), max_depth, depth + 1);
        bottom = std::make_unique<BVH>(std::make_pair(glm::vec2(m_min_coord.x, split_y), m_max_coord), max_depth, depth + 1);
    }
}

void BVH::add_way(std::shared_ptr<Way> way) {
    auto& [ a, b ] = m_children;

    if(!a || !b) {
        m_ways.push_back(std::move(way));
        return;
    }

    bool in_a = way->intersects(*a);
    bool in_b = way->intersects(*b);

    if(in_a == in_b)
        m_ways.push_back(std::move(way));
    else if(in_a)
        a->add_way(std::move(way));
    else if(in_b)
        b->add_way(std::move(way));
    else
        assert(false && "unreachable");
}

void BVH::draw(BBox& viewport, RenderFlags flags, size_t max_depth, size_t depth)
{
    if(depth >= max_depth)
        return;
    
    for(auto& way : m_ways) {
        way->draw_buffers(flags);
    }

    auto& [ a, b ] = m_children;
    if(a != nullptr && a->intersects(viewport))
        a->draw(viewport, flags, max_depth, depth + 1);
    if(b != nullptr && b->intersects(viewport))
        b->draw(viewport, flags, max_depth, depth + 1);
}

std::pair<float, std::shared_ptr<Way>> BVH::get_nearest_way(glm::vec2 coords) const {
    float min_dist = std::numeric_limits<float>::infinity();
    std::shared_ptr<Way> way_ptr = nullptr;

    auto& [ a, b ] = m_children;
    if(a != nullptr && a->contains(coords)) {
        auto inner = a->get_nearest_way(coords);
        min_dist = inner.first;
        way_ptr = inner.second;
    }
    else if(b != nullptr && b->contains(coords)) {
        auto inner = b->get_nearest_way(coords);
        min_dist = inner.first;
        way_ptr = inner.second;
    }

    for(auto& way : m_ways) {
        if(!way->contains(coords))
            continue;

        for(auto& node : way->get_nodes()) {
            auto dist = glm::distance2(coords, node.m_coord);
            if(dist < min_dist) { 
                min_dist = dist;
                way_ptr = way;
            }
        }
    }
    
    return std::make_pair(min_dist, way_ptr);
}

