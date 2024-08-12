#include "bvh.hpp"

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
        auto& [ a, b ] = m_children;

        a = std::make_unique<BVH>(std::make_pair(m_min_coord, glm::vec2(split_x, m_max_coord.y)), max_depth, depth + 1);
        b = std::make_unique<BVH>(std::make_pair(glm::vec2(split_x, m_min_coord.y), m_max_coord), max_depth, depth + 1);
    }
    else {
        // split horizontal

        float split_y = m_min_coord.y + size.y / 2.0f;
        auto& [ a, b ] = m_children;

        a = std::make_unique<BVH>(std::make_pair(m_min_coord, glm::vec2(m_max_coord.x, split_y)), max_depth, depth + 1);
        b = std::make_unique<BVH>(std::make_pair(glm::vec2(m_min_coord.x, split_y), m_max_coord), max_depth, depth + 1);
    }
}

void BVH::add_way(std::unique_ptr<Way> way) {
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

void BVH::draw(BBox& viewport, size_t max_depth, size_t depth)
{
    if(depth >= max_depth)
        return;
    
    for(auto& way : m_ways) {
        way->draw_buffers();
    }

    auto& [ a, b ] = m_children;
    if(a != nullptr && a->intersects(viewport))
        a->draw(viewport, max_depth, depth + 1);
    if(b != nullptr && b->intersects(viewport))
        b->draw(viewport, max_depth, depth + 1);
}

