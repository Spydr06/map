#pragma once

#include "bbox.hpp"

#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>
#include <cairo.h>

#include "bvh.hpp"
#include "inputstate.hpp"
#include "renderutil.hpp"
#include "inspector.hpp"
#include "way.hpp"

struct CachedTag {
    enum State {
        TAG_STATE_UNLOADED = 0,
        TAG_STATE_FETCHED,
        TAG_STATE_RENDERED,
        TAG_STATE_LOADED
    };

    CachedTag() : m_icon_url("") {}
    CachedTag(std::string& icon_url)
        : m_icon_url(icon_url)
    {}

    int load_image();

    State m_state = TAG_STATE_UNLOADED;
    std::string m_icon_url;
    std::string m_svg_path;
    cairo_surface_t* m_cairo_surface;

    GLuint m_texture_id;
};

class Map : public BBox, public RenderElement {
public:
    Map();
    
    void init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth);

    void register_taginfo(std::string& key, std::string& value, std::string& icon_url);

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
        return m_bvh->get_nearest_way(coords, m_draw_priority);
    }

    std::unordered_map<std::string, std::unordered_map<std::string, CachedTag>> m_taginfo{};

private:
    std::unique_ptr<BVH> m_bvh;
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Shader> m_selection_shader;
    
    Inspector m_inspector;

    std::size_t m_max_bvh_depth, m_render_bvh_depth;
    std::shared_ptr<Way> m_selected_way;

    DrawPriority m_draw_priority = DrawPriority::__DRAW_PRIO_LAST;
};

