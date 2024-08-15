#include "bvh.hpp"
#include "way.hpp"

#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

BVH::BVH(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth, size_t depth)
    : BBox(minmax_coords), m_children(std::make_pair(nullptr, nullptr)), m_ways()
{
    if(depth + 1 >= max_depth)
        return;

    for(int i = 0; i < DrawPriority::__DRAW_PRIO_LAST; i++) {
        m_ways[i] = std::vector<std::shared_ptr<Way>>();
    }

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

    auto priority = way->get_metadata().draw_priority();
    if(!a || !b) {
        m_ways[priority].push_back(std::move(way));
        return;
    }

    bool in_a = way->intersects(*a);
    bool in_b = way->intersects(*b);

    if(in_a == in_b)
        m_ways[priority].push_back(std::move(way));
    else if(in_a)
        a->add_way(std::move(way));
    else if(in_b)
        b->add_way(std::move(way));
}

void BVH::draw(BBox& viewport, DrawPriority priority, size_t max_depth, size_t depth)
{
    if(depth >= max_depth)
        return;
    
    for(int i = 0; i < static_cast<int>(priority); i++) {
        auto& ways = m_ways[i];
        for(auto& way : ways) {
            way->draw_buffers();
        }
    }

    auto& [ a, b ] = m_children;
    if(a != nullptr && a->intersects(viewport))
        a->draw(viewport, priority, max_depth, depth + 1);
    if(b != nullptr && b->intersects(viewport))
        b->draw(viewport, priority, max_depth, depth + 1);
}

std::pair<float, std::shared_ptr<Way>> BVH::get_nearest_way(glm::vec2 coords, DrawPriority priority) const {
    float min_dist = std::numeric_limits<float>::infinity();
    std::shared_ptr<Way> way_ptr = nullptr;

    auto& [ a, b ] = m_children;
    if(a != nullptr && a->contains(coords)) {
        auto inner = a->get_nearest_way(coords, priority);
        min_dist = inner.first;
        way_ptr = inner.second;
    }
    else if(b != nullptr && b->contains(coords)) {
        auto inner = b->get_nearest_way(coords, priority);
        min_dist = inner.first;
        way_ptr = inner.second;
    }

    for(int i = 0; i < priority; i++) {
        for(auto& way : m_ways[i]) {
            for(size_t j = 1; j < way->get_nodes().size(); j++) {
                auto v = way->get_nodes()[j - 1].m_coord;
                auto w = way->get_nodes()[j].m_coord;

                float length_sq = glm::distance2(v, w);
                float dist_sq = 0.0f;
                if(length_sq == 0.0f)
                    dist_sq = glm::distance2(coords, v);
                else {
                    float t = std::max(0.0f, std::min(1.0f, glm::dot(coords - v, w - v) / length_sq));
                    auto projection = v + t * (w - v);
                    dist_sq = glm::distance2(coords, projection);
                }

                if(dist_sq < min_dist) {
                    min_dist = dist_sq;
                    way_ptr = way;
                }
            }
        }
    }
    
    return std::make_pair(min_dist, way_ptr);
}

