#pragma once

#include "renderutil.hpp"
#include "contour.hpp"

#include <xtiffio.h>
#include <geotiff.h>

#include <map>
#include <vector>

#include <glm/vec4.hpp>

#include "log.hpp"
#include "way.hpp"

#define DEFAULT_PIXELS_PER_METER 30

class Heightmap;

class HeightmapRenderMode {
public:
    HeightmapRenderMode(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
    ~HeightmapRenderMode() = default;

    virtual void begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) = 0;
    virtual void draw_ui(Heightmap& heightmap) {}
protected:
    std::unique_ptr<Shader> m_shader;
};

class HeightmapAltitudeMode : public HeightmapRenderMode {
public: 
    HeightmapAltitudeMode()
        : HeightmapRenderMode("./shaders/altitude_vertex.glsl", "./shaders/altitude_fragment.glsl")
    {}

    virtual void begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) override;
};

class HeightmapGradientMode : public HeightmapRenderMode {
public:
    HeightmapGradientMode()
        : HeightmapRenderMode("./shaders/gradient_vertex.glsl", "./shaders/gradient_fragment.glsl"),
          m_brightness(0.1f), m_delta(0.01f)
    {}

    virtual void begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) override;
    virtual void draw_ui(Heightmap& heightmap) override;
private:
    non_volatile<float, "heightmap.gradient_brightness"> m_brightness;
    non_volatile<float, "heightmap.gradient_delta"> m_delta;
}; 

class HeightmapContourMode : public HeightmapRenderMode {
public:
    HeightmapContourMode()
        : HeightmapRenderMode("./shaders/contour_vertex.glsl", "./shaders/contour_fragment.glsl"),
          m_color(1.0f), m_epsilon(0.1f), m_spacing(50.0f), m_hl_frequency(5), m_hl_multiply(1.6f)
    {}

    virtual void begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) override;
    virtual void draw_ui(Heightmap& heightmap) override;
private:
    non_volatile<glm::vec4, "heightmap.contour_color"> m_color;
    non_volatile<float, "heightmap.contour_epsilon"> m_epsilon;
    non_volatile<float, "heightmap.contour_spacing"> m_spacing;
    non_volatile<int, "heightmap.contour_hl_frequency"> m_hl_frequency;
    non_volatile<float, "heightmap.contour_hl_multiply"> m_hl_multiply;
};

class HeightmapTile {
public:
    HeightmapTile(std::vector<float> pixels, uint32_t width, uint32_t height, double min_lon, double min_lat, double max_lon, double max_lat);
    ~HeightmapTile();

    void create_texture();
    void create_buffers();
    void draw_buffers();

    bool contains_position(glm::vec2 const& pos) const;
    std::optional<float> altitude_at_position(glm::vec2 const& pos) const;

    inline GLuint texture() const {
        return m_texture;
    }

private:
    GLuint m_texture = 0;
    GLuint m_vao = 0, m_vbo = 0;

    uint32_t m_width, m_height;

    std::vector<float> m_pixels;
    glm::vec2 m_min_min, m_min_max, m_max_min, m_max_max;
};

class Heightmap : public RenderElement, public ViewportProvider {
public:
    Heightmap(std::string path);

    ~Heightmap() {
        GTIFFree(m_gtif);
        XTIFFClose(m_tif);
    }

    int preprocess();
    void create_buffers();

    std::optional<std::shared_ptr<HeightmapTile>> get_tile(uint32_t x, uint32_t y);

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;

    virtual void on_attach(RenderContext& context) override;

    virtual int get_z_index() const {
        return 2;
    }

    virtual bool remove() const {
        return m_remove;
    }

    virtual std::pair<glm::vec2, glm::vec2> get_minmax_coord() const override {
        return {
            map_project({m_info.min_lon, m_info.min_lat}),
            map_project({m_info.max_lon, m_info.max_lat})
        };
    }

    std::optional<std::shared_ptr<HeightmapTile>> tile_at_position(const glm::vec2& pos) const;
    std::optional<float> altitude_at_position(const glm::vec2& pos) const;
    std::vector<float> altitude_graph(const Way& way) const;

    inline constexpr uint32_t get_tile_index(uint32_t x, uint32_t y) const {
        return x * (m_info.height / m_info.tile_height) + y;
    }

    std::pair<float, float> get_height_range() const {
        return std::pair(m_info.min_height, m_info.max_height);
    }

    struct {
        uint32_t width, height;
        uint32_t tile_width, tile_height;

        double scale_lon, scale_lat;
        double min_lon, max_lon;
        double min_lat, max_lat;
        float min_height, max_height;
    } m_info;

private:
    std::string m_tif_path;
    TIFF *m_tif;
    GTIF *m_gtif;

    std::map<uint32_t, std::shared_ptr<HeightmapTile>> m_tiles;
    std::map<uint32_t, std::shared_ptr<ContourTile>> m_contours;

    std::map<std::string, std::shared_ptr<HeightmapRenderMode>> m_modes;
    non_volatile<std::string, "heightmap.mode"> m_current_mode;

    GLuint m_tile_ebo = 0;
    bool m_remove = false;
};

