#pragma once

#include "bbox.hpp"

#include <atomic>
#include <expected>
#include <future>
#include <memory>

#include <glm/vec2.hpp>
#include <GL/glew.h>
#include <cairo.h>
#include <optional>

#include "bvh.hpp"
#include "inputstate.hpp"
#include "renderutil.hpp"
#include "maptools.hpp"
#include "non_volatile.hpp"
#include "way.hpp"
#include "heightmap.hpp"

class Progress {
public:
    Progress(std::string unit = "MiB")
        : m_unit(unit)
    {}

    void draw_progress_bar() const;
    void update(float progress);
    void set_total(float total);

private:
    std::atomic<float> m_total = 1.0;
    std::atomic<float> m_progress = 0.0;
    std::string m_unit;
};


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

    void rebuild_vaos();

    CachedTag* get_taginfo(const std::string& key, const std::string& value);
    std::vector<CachedTag*> get_taginfos(const std::string& key, const std::string& value);

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;
    virtual void menu_item() override;

    inline void mark_removal() {
        m_remove = true;
    }

    virtual bool remove() const override {
        return m_remove;
    }

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

    inline void set_draw_priority(DrawPriority priority) {
        m_draw_priority = priority;
    }

    inline auto get_draw_priority() {
        return m_draw_priority;
    }

    inline void set_auto_priority(bool v) {
        m_auto_priority = v;
    }

    inline auto get_auto_priority() {
        return m_auto_priority;
    }

    inline void deselect_tool() {
        m_selected_tool = std::nullopt;
    }

    inline void set_source(std::string source) {
        m_source = source;
    }

    inline std::string get_source() {
        return m_source;
    }

private:
    std::string m_source;
    
    std::shared_ptr<Heightmap> m_heightmap = nullptr;

    std::unique_ptr<BVH> m_bvh;
    std::unique_ptr<Shader> m_shader;

    std::map<std::string, std::unique_ptr<MapTool>> m_tools;
    std::optional<std::string> m_selected_tool;
    
    std::size_t m_max_bvh_depth, m_render_bvh_depth;

    DrawPriority m_draw_priority = DrawPriority::__DRAW_PRIO_LAST;
    bool m_auto_priority = true;
    bool m_remove = false;
};

class LoaderContext;

class MapLoader : public RenderElement {
public:
    MapLoader()
        : m_loading_map(std::nullopt)
    {}

    ~MapLoader() = default;

    virtual void menu_item() override;

    virtual void draw_scene(Viewport& viewport, InputState& input) override {};
    virtual void draw_ui(InputState& input) override;

    virtual int get_z_index() const override {
        return -1;
    }

private:
    Progress m_loading_progress{};
    std::optional<std::future<std::expected<std::shared_ptr<Map>, int>>> m_loading_map;

    friend std::expected<std::shared_ptr<Map>, int> load_map(std::string, std::shared_ptr<Map>, std::unique_ptr<LoaderContext>, MapLoader*);
};

class MapTheme {
public:
    static constexpr auto N = static_cast<std::size_t>(Metadata::__CLASSIFICATION_LAST);
    static constexpr auto INVAL_COLOR = glm::vec4(1.0, 0.0, 1.0, 1.0);

    MapTheme(std::unordered_map<Metadata::Classification, glm::vec4> init, glm::vec3 background)
        : m_background(background)
    {
        m_entries.fill(INVAL_COLOR);

        for(auto [class_, color] : init) {
            m_entries[static_cast<std::size_t>(class_)] = color;
        }
    }

    void use(const Shader& shader);

    inline auto operator[](Metadata::Classification classification) -> std::optional<glm::vec4> const {
        if(classification < 0 || static_cast<std::size_t>(classification) >= N)
            return std::nullopt;
        return m_entries[classification];
    }

    inline auto operator*() -> std::array<glm::vec4, N>& {
        return m_entries;
    }

    inline glm::vec3& background() {
        return m_background;
    }

protected:
    std::array<glm::vec4, N> m_entries;
    glm::vec3 m_background;
};

class PresetTheme : public MapTheme {
public:
    PresetTheme(glm::vec4 primary, glm::vec4 secundary, glm::vec4 water, glm::vec4 foliage, std::array<glm::vec4, 3> accent, glm::vec3 m_background, glm::vec4 trans = glm::vec4(0.0));
};

class MapView : public RenderElement {
public:
    MapView() { load_presets(); }

    void load_presets();

    inline std::shared_ptr<MapTheme> get_theme() {
        return m_presets[m_theme];
    }

    virtual void menu_item() override;

    virtual void draw_scene(Viewport& viewport, InputState& input) override {};
    virtual void draw_ui(InputState& input) override;

    virtual int get_z_index() const override {
        return -1;
    }

private:
    bool m_editing;

    non_volatile<std::string, "settings.theme"> m_theme{"Sage"};
    std::unordered_map<std::string, std::shared_ptr<MapTheme>> m_presets;
};

