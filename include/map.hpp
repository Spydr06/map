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
#include "heightmap.hpp"

struct CachedTag {
    enum State {
        TAG_STATE_UNLOADED = 0,
        TAG_STATE_FETCHED,
        TAG_STATE_LOADED,
        TAG_STATE_PRESENT
    };

    enum Format {
        TAG_FORMAT_UNKNOWN = 0,
        TAG_FORMAT_SVG,
        TAG_FORMAT_PNG
    };

    int load_image();

    CachedTag()
        : m_dimensions(128.f, 128.f)
    {}

private:
    int fetch();
    int read_svg();
    int read_png();
    int create_texture();

    inline auto image_format() -> Format {
        auto dot_pos = m_icon_path.find_last_of('.');
        if(dot_pos == std::string::npos)
            return TAG_FORMAT_UNKNOWN;

        std::string ext = m_icon_path.substr(dot_pos + 1);
        if(ext == "svg")
            return TAG_FORMAT_SVG;
        else if(ext == "png")
            return TAG_FORMAT_PNG;
        else
            return TAG_FORMAT_UNKNOWN;
    }

public:
    State m_state = TAG_STATE_UNLOADED;

    std::string m_icon_url;
    std::string m_icon_path;
    cairo_surface_t* m_cairo_surface;

    glm::vec2 m_dimensions;
    GLuint m_texture_id;
};

class Map : public BBox, public RenderElement {
public:
    Map();
    
    void init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth);

    void register_taginfo(std::string& key, std::string& value, std::string& icon_url);

    CachedTag* get_taginfo(const std::string& key, const std::string& value);
    std::vector<CachedTag*> get_taginfos(const std::string& key, const std::string& value);

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

    inline auto set_heightmap(std::shared_ptr<Heightmap> heightmap) {
        m_heightmap = heightmap;
    }

private:
    std::optional<std::shared_ptr<Heightmap>> m_heightmap{};

    std::unique_ptr<BVH> m_bvh;
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Shader> m_selection_shader;
    
    Inspector m_inspector;

    std::size_t m_max_bvh_depth, m_render_bvh_depth;
    std::shared_ptr<Way> m_selected_way;

    DrawPriority m_draw_priority = DrawPriority::__DRAW_PRIO_LAST;
};

