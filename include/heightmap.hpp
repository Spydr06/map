#pragma once

#include "renderutil.hpp"
#include "contour.hpp"

#include <xtiffio.h>
#include <geotiff.h>

#include <map>
#include <vector>

#include "log.hpp"

#define DEFAULT_PIXELS_PER_METER 30

class HeightmapTile {
public:
    HeightmapTile(std::vector<float> pixels, uint32_t width, uint32_t height, double min_lon, double min_lat, double max_lon, double max_lat)
        : m_width(width), m_height(height), m_pixels(pixels),
          m_start(map_project(glm::vec2(min_lon, min_lat))), 
          m_end(map_project(glm::vec2(max_lon, max_lat)))
    {
        auto [min, max] = std::minmax_element(m_pixels.begin(), m_pixels.end());
        mlog::logln(mlog::INFO, "min height: %f, max height: %f", *min, *max);
        mlog::logln(mlog::INFO, "mapped: [%f, %f -> %f, %f]", m_start.x, m_start.y, m_end.x, m_end.y);

        create_texture();
        create_buffers();
    }

    void create_texture();
    void create_buffers();
    void draw_buffers();

    bool contains_pos(glm::vec2 const& pos) const {
        return pos.x >= m_start.x && pos.x < m_end.x && pos.y <= m_start.y && pos.y > m_end.y;
    }

    float height_at_pos(glm::vec2 const& pos) const {
        if(!contains_pos(pos))
            return -1.f;

        double x = ((pos.x - m_start.x) / (m_end.x - m_start.x)) * m_width;
        double y = ((pos.y - m_end.y) / (m_start.y - m_end.y)) * m_height;

        return m_pixels[static_cast<uint32_t>(y * m_width + x)];
    }
private:
    GLuint m_texture;
    GLuint m_vao = 0, m_vbo = 0;
    uint32_t m_width, m_height;

    std::vector<float> m_pixels;
    glm::vec2 m_start, m_end;
};

class Heightmap : public RenderElement {
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

    inline constexpr uint32_t get_tile_index(uint32_t x, uint32_t y) const {
        return x * (m_info.height / m_info.tile_height) + y;
    }

    std::optional<std::shared_ptr<HeightmapTile>> get_tile_at_position(glm::vec2 const& pos) const {
        for(auto it = m_tiles.begin(); it != m_tiles.end(); it++) {
            if(it->second->contains_pos(pos))
                return it->second;
        }

        return {};
    }
private:
    std::string m_tif_path;
    TIFF *m_tif;
    GTIF *m_gtif;

    std::map<uint32_t, std::shared_ptr<HeightmapTile>> m_tiles;
    std::map<uint32_t, std::shared_ptr<ContourTile>> m_contours;

    std::unique_ptr<Shader> m_shader;

    struct {
        uint32_t width, height;
        uint32_t tile_width, tile_height;

        double scale_lon, scale_lat;
        double min_lon, max_lon;
        double min_lat, max_lat;
        float min_height, max_height;
    } m_info;

    GLuint m_tile_ebo = 0;
};

