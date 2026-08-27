#include "heightmap.hpp"

#include "log.hpp"
#include "main.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "viewport.hpp"

#include <fstream>

#include <geokeys.h>
#include <memory>
#include <tiff.h>
#include <tiffio.h>
#include <xtiffio.h>
#include <geotiff.h>
#include <geotiffio.h>
#include <geo_tiffp.h>
#include <geo_normalize.h>
#include <proj.h>

#include <imgui.h>

static constexpr GLuint indices[] = { 0, 1, 2, 2, 1, 3 };

Heightmap::Heightmap(std::string path)
        : m_tif_path(path), m_tif{}, m_modes{}
{
    m_modes["Altitude"] = std::make_shared<HeightmapAltitudeMode>();
    m_modes["Gradient"] = std::make_shared<HeightmapGradientMode>();
    m_modes["Contours"] = std::make_shared<HeightmapContourMode>();

    m_current_mode = std::pair("Altitude", m_modes["Altitude"]);

    create_buffers();
}

void Heightmap::create_buffers() {
    glGenBuffers(1, &m_tile_ebo);
    assert(m_tile_ebo != 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tile_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

int Heightmap::preprocess() {
    mlog::logln(mlog::INFO, "Loading Heightmap %s...", m_tif_path.c_str());

    if(!(m_tif = XTIFFOpen(m_tif_path.c_str(), "r"))) {
        mlog::logln(mlog::ERROR, "Error loading geotiff %s.", m_tif_path.c_str());
        return 1;
    }

    if(!(m_gtif = GTIFNew(m_tif))) {
        mlog::logln(mlog::ERROR, "Error loading geotiff %s.", m_tif_path.c_str());
        return 1;
    }

    geocode_t model;

    if(!GTIFKeyGet(m_gtif, GTModelTypeGeoKey, &model, 0, 1)) {
        mlog::logln(mlog::ERROR, "Error loading geotiff model.");
        return 1;
    }

    TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &m_info.width);
    TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &m_info.height);
    TIFFGetField(m_tif, TIFFTAG_TILEWIDTH, &m_info.tile_width);
    TIFFGetField(m_tif, TIFFTAG_TILELENGTH, &m_info.tile_height);

    mlog::logln(mlog::INFO, "[%u x %u] pixels", m_info.width, m_info.height);

    double *scale = nullptr, *tiepoints = nullptr;
    uint16_t scale_count = 0, tie_count = 0;

    if(!TIFFGetField(m_tif, TIFFTAG_GEOPIXELSCALE, &scale_count, &scale)) {
        mlog::logln(mlog::ERROR, "Could not get geopixel scale.");
        return 1;
    }

    if(!TIFFGetField(m_tif, TIFFTAG_GEOTIEPOINTS, &tie_count, &tiepoints)) {
        mlog::logln(mlog::ERROR, "Could not get tiepoints.");
        return 1;
    }

    mlog::logln(mlog::INFO, "scale: %d [%f x %f x %f]", scale_count, scale[0], scale[1], scale[2]);
    mlog::logln(mlog::INFO, "tie: %d", tie_count);

    double *matrix = nullptr;
    uint16_t matrix_count = 0;
    TIFFGetField(m_tif, TIFFTAG_MODELTRANSFORMATIONTAG, &matrix_count, matrix);
    if(matrix_count > 0) {
        mlog::logln(mlog::ERROR, "Matrix model transforms are not supported.");
        return 1;
    }

    m_info.scale_lon = scale[0];
    m_info.scale_lat = scale[1];
    m_info.min_lon = tiepoints[3];
    m_info.min_lat = tiepoints[4];
    m_info.max_lon = m_info.min_lon + m_info.width * scale[0];
    m_info.max_lat = m_info.min_lat - m_info.height * scale[1];

    // TODO: CRS check
    GTIFDefn defn;
    if(!GTIFGetDefn(m_gtif, &defn)) {
        mlog::logln(mlog::ERROR, "Could not get geotiff defn");
        return 1;
    }

    char *proj4 = GTIFGetProj4Defn(&defn);
    mlog::logln(mlog::INFO, "projection model: %s", proj4);

    mlog::logln(mlog::INFO, "lon: %f - %f", m_info.min_lon, m_info.max_lon);
    mlog::logln(mlog::INFO, "lat: %f - %f", m_info.min_lat, m_info.max_lat);

    return 0;
}

std::optional<std::shared_ptr<HeightmapTile>> Heightmap::get_tile(uint32_t x, uint32_t y) {
    if(x > m_info.width / m_info.tile_width
        || y > m_info.height / m_info.tile_height)
        return {};

    auto tile = m_tiles.find(get_tile_index(x, y));
    if(tile != m_tiles.end())
        return tile->second;

    double min_lon = m_info.min_lon + x * m_info.tile_width * m_info.scale_lon;
    double max_lon = min_lon + m_info.tile_width * m_info.scale_lon;

    double min_lat = m_info.min_lat - y * m_info.tile_height * m_info.scale_lat;
    double max_lat = min_lat - m_info.tile_height * m_info.scale_lat;

    mlog::logln(mlog::INFO, "generating heightmap tile (%d, %d) [%f, %f -> %f, %f]", x, y, min_lon, min_lat, max_lon, max_lat);

    std::vector<float> pixels(TIFFTileSize(m_tif) / sizeof(float));
    TIFFReadTile(m_tif, reinterpret_cast<void*>(pixels.data()), x * m_info.tile_width, y * m_info.tile_height, 0, 0);

    auto [min_height, max_height] = std::minmax_element(pixels.begin(), pixels.end());
    mlog::logln(mlog::INFO, "min height: %f, max height: %f", *min_height, *max_height);

    if(*min_height < m_info.min_height)
        m_info.min_height = *min_height;
    if(*max_height > m_info.max_height)
        m_info.max_height = *max_height;

    return m_tiles[get_tile_index(x, y)] = std::make_shared<HeightmapTile>(pixels, m_info.tile_width, m_info.tile_height, min_lon, min_lat, max_lon, max_lat);
}

void Heightmap::draw_scene(Viewport& viewport, InputState& input) {
    m_current_mode.second->begin_render(*this, viewport, input);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tile_ebo);

    for(uint32_t y = 0; y < m_info.height / m_info.tile_height; y++) {
        for(uint32_t x = 0; x < m_info.width / m_info.tile_width; x++) {
            if(auto tile = get_tile(x, y)) {
                (*tile)->draw_buffers();
            }
        }
    }
}

void Heightmap::draw_ui(InputState& input) {
    ImGui::Begin("Heightmap");

    ImGui::Text("Height min: %f, max: %f Meters", m_info.min_height, m_info.max_height);

    glm::vec2 cursor = input.mapped_cursor_pos;
    if(auto tile = get_tile_at_position(cursor))
        ImGui::Text("Height at (%f, %f): %f Meters", cursor.x, cursor.y, (*tile)->height_at_pos(cursor));
    else
        ImGui::Text("Height at (%f, %f): ---", cursor.x, cursor.y);

    ImGui::Separator();

    if(ImGui::BeginCombo("Shader", m_current_mode.first.c_str())) {
        for(auto& [name, mode] : m_modes) {
            bool is_selected = (m_current_mode.second == mode);

            if(ImGui::Selectable(name.c_str(), is_selected))
                m_current_mode = std::pair(name, mode);

            if(is_selected)
                ImGui::SetItemDefaultFocus();

        }
        ImGui::EndCombo();
    }

    m_current_mode.second->draw_ui(*this);

    ImGui::End();
}

void HeightmapTile::create_texture() {
    mlog::logln(mlog::INFO, "tile %d x %d, %zu", m_width, m_height, m_pixels.size());
    glGenTextures(1, &m_texture);
    assert(m_texture != 0);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, m_pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLfloat value, max_anisotropy = 8.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &value);
    mlog::logln(mlog::INFO, "anisotropy max: %f", value);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(max_anisotropy, value));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void HeightmapTile::create_buffers() {
    struct vertex {
        glm::vec2 vertex;
        glm::vec2 tex_coord;
    } vertices[] = {
        { glm::vec2(m_start),            glm::vec2(0, 0) },
        { glm::vec2(m_start.x, m_end.y), glm::vec2(0, 1) },
        { glm::vec2(m_end.x, m_start.y), glm::vec2(1, 0) },
        { glm::vec2(m_end),              glm::vec2(1, 1) }
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    assert(m_vao != 0);
    assert(m_vbo != 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(m_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*) offsetof(struct vertex, vertex));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void*) offsetof(struct vertex, tex_coord));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void HeightmapTile::draw_buffers() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glBindVertexArray(m_vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
}

HeightmapRenderMode::HeightmapRenderMode(const std::string& vertex_shader_path, const std::string& fragment_shader_path) {
    auto vertex_source = std::ifstream(vertex_shader_path);
    auto fragment_source = std::ifstream(fragment_shader_path);
    if(vertex_source.bad() || fragment_source.bad()) {
        mlog::logln(mlog::ERROR, "Shader error: %s: Shader file not found", vertex_shader_path.c_str());
        std::exit(1);
    }

    m_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_shader->get_error()) {
        mlog::logln(mlog::ERROR, "Shader error: %s: %s", vertex_shader_path.c_str(), err->c_str());
        std::exit(1);
    }
}

void HeightmapGradientMode::begin_render(Heightmap&, Viewport& viewport, InputState& input) {
    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);
    m_shader->upload_uniform("u_Texture", 0u);
    m_shader->upload_uniform("u_Delta", m_delta);
    m_shader->upload_uniform("u_Brightness", m_brightness);
}

void HeightmapGradientMode::draw_ui(Heightmap&) {
    ImGui::SliderFloat("Gradient Delta", &m_delta, 0.0, 1.0);
    ImGui::SliderFloat("Brightness", &m_brightness, 0.0, 1.0);
}

void HeightmapAltitudeMode::begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) {
    auto [min_height, max_height] = heightmap.get_height_range();

    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);
    m_shader->upload_uniform("u_Texture", 0u);
    m_shader->upload_uniform("u_HeightRange", glm::vec2(min_height, max_height));
}

void HeightmapContourMode::begin_render(Heightmap& heightmap, Viewport& viewport, InputState& input) {
    auto [min_height, max_height] = heightmap.get_height_range();

    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);
    m_shader->upload_uniform("u_Texture", 0u);
    m_shader->upload_uniform("u_Epsilon", m_epsilon);
    m_shader->upload_uniform("u_Spacing", m_spacing);
    m_shader->upload_uniform("u_HeightRange", glm::vec2(min_height, max_height));
    m_shader->upload_uniform("u_Color", m_color);
    m_shader->upload_uniform("u_BackgroundColor", context->get_clear_color());
}

void HeightmapContourMode::draw_ui(Heightmap& heightmap) {
    ImGui::SliderFloat("Spacing [m]", &m_spacing, 1.0, 500);
    ImGui::SliderFloat("Epsilon", &m_epsilon, 0.0, 5.0);

    if(ImGui::TreeNode("Color")) {
        ImGui::ColorEdit4("", reinterpret_cast<float*>(&m_color));
        
        ImGui::TreePop();
    }
}

