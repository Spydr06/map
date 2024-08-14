#pragma once

#include "bbox.hpp"

#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>

#include "bvh.hpp"
#include "inputstate.hpp"
#include "renderutil.hpp"

class Map : public BBox, public RenderElement {
public:
    Map();
    
    void init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth);

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;

    inline void add_way(std::shared_ptr<Way> way) {
        assert(m_bvh);
        m_bvh->add_way(std::move(way));
    }

    inline auto get_max_bvh_depth() const -> std::size_t {
        return m_max_bvh_depth;
    }

    inline auto get_nearest_way(glm::vec2 coords) const -> std::pair<float, std::shared_ptr<Way>> {
        return m_bvh->get_nearest_way(coords);
    }
    
private:
    std::unique_ptr<BVH> m_bvh;
    std::unique_ptr<Shader> m_shader;
    std::size_t m_max_bvh_depth, m_render_bvh_depth;
};

