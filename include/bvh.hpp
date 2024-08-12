#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "bbox.hpp"
#include "way.hpp"

class BVH : public BBox {
public:
    BVH(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth, size_t depth);

    void add_way(std::unique_ptr<Way> way);
    void draw(BBox& viewport, size_t max_depth, size_t depth);

private:
    std::pair<std::unique_ptr<BVH>, std::unique_ptr<BVH>> m_children;
    std::vector<std::unique_ptr<Way>> m_ways;
};

