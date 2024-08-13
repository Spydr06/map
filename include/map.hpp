#pragma once

#include "bbox.hpp"

#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>

#include "bvh.hpp"
#include "flags.hpp"

class Map : public BBox {
public:
    Map()
        : m_bvh(nullptr) 
    {}

    void init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth) {
        assert(!m_bvh);

        set_minmax_coord(minmax_coords);
        m_max_bvh_depth = max_depth;
        m_bvh = std::make_unique<BVH>(minmax_coords, max_depth, 0);
    }

    inline void add_way(std::shared_ptr<Way> way) {
        assert(m_bvh);
        m_bvh->add_way(std::move(way));
    }

    inline void draw(BBox& viewport, RenderFlags flags, size_t max_depth) {
        m_bvh->draw(viewport, flags, max_depth, 0);
    }

    inline auto get_max_bvh_depth() const -> std::size_t {
        return m_max_bvh_depth;
    }

    inline auto get_nearest_way(glm::vec2 coords) const {
        return m_bvh->get_nearest_way(coords);
    }
    
private:
    std::unique_ptr<BVH> m_bvh;
    std::size_t m_max_bvh_depth;
};

