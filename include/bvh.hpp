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

    void add_way(std::shared_ptr<Way> way);
    void draw(BBox& viewport, DrawPriority priority, size_t max_depth, size_t depth);

    std::pair<float, std::shared_ptr<Way>> get_nearest_way(glm::vec2 coords) const;

private:
    std::pair<std::unique_ptr<BVH>, std::unique_ptr<BVH>> m_children;
    std::vector<std::shared_ptr<Way>> m_ways[__DRAW_PRIO_LAST];
};

