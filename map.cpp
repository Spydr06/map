#include "map.hpp"
#include "inspector.hpp"
#include "preprocess.hpp"
#include "rendercontext.hpp"
#include "screenshot.hpp"
#include "way.hpp"
#include "log.hpp"
#include "renderutil.hpp"
#include "main.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>

#include <future>
#include <imgui.h>
#include <memory>
#include <nfd.h>

static const PresetTheme NAVY_THEME(
    glm::vec4(0.439, 0.412, 0.576, 1.0),
    glm::vec4(0.627, 0.757, 0.725, 1.0),
    glm::vec4(0.439, 0.627, 0.686, 1.0),
    glm::vec4(0.757, 0.788, 0.729, 1.0),
    std::array<glm::vec4, 3>{
        glm::vec4(0.2, 0.118, 0.22, 1.0),
        glm::vec4(0.2, 0.118, 0.22, 1.0),
        glm::vec4(0.2, 0.118, 0.22, 1.0)
    },
    glm::vec3(1.0f, 0.953f, 0.914f)
);

static const PresetTheme SAGE_THEME(
    glm::vec4(0.322,0.475,0.435, 1.0),
    glm::vec4(0.322,0.475,0.435, 1.0),
    glm::vec4(0.518,0.663,0.549, 1.0),
    glm::vec4(0.208,0.31,0.322, 1.0),
    std::array<glm::vec4, 3>{
        glm::vec4(0.929,0.416,0.353, 1.0),
        glm::vec4(0.792,0.824,0.773, 1.0),
        glm::vec4(0.792,0.824,0.773, 1.0)
    },
    glm::vec3(0.184,0.243,0.275)
);

static const PresetTheme GRAYSCALE_THEME(
    glm::vec4(1.0, 1.0, 1.0, 1.0),
    glm::vec4(1.0, 1.0, 1.0, 1.0),
    glm::vec4(1.0, 1.0, 1.0, 1.0),
    glm::vec4(0.3, 0.3, 0.3, 1.0),
    std::array<glm::vec4, 3>{
        glm::vec4(1.0, 1.0, 1.0, 1.0),
        glm::vec4(1.0, 1.0, 1.0, 1.0),
        glm::vec4(1.0, 1.0, 1.0, 1.0)
    },
    glm::vec3(0.0, 0.0, 0.0)
);


void Progress::draw_progress_bar() const {
    auto total = m_total.load();
    auto progress = m_progress.load();
    auto frac = progress / total; 

    std::string s = std::format("{:.1f} of {:.1f} {} ({:.1f}%)", progress, total, m_unit, frac * 100.0f);
    ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), s.c_str());
}

void Progress::update(float progress) {
    m_progress = progress;
}

void Progress::set_total(float total) {
    m_total = total;
}


Map::Map()
    : m_bvh(nullptr), m_tools{}
{
    auto vertex_source = std::ifstream("shaders/map_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/map_fragment.glsl");
    if(vertex_source.bad() || fragment_source.bad()) {
        mlog::logln(mlog::ERROR, "Shader error: Shader file not found");
        std::exit(1);
    }

    m_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_shader->get_error()) {
        mlog::logln(mlog::ERROR, "Shader error: %s", err->c_str());
        std::exit(1);
    }


    m_tools.emplace("Inspect", std::make_unique<Inspector>());
    m_tools.emplace("Select (Rect)", std::make_unique<RectangleSelect>());
    m_tools.emplace("Screenshot", std::make_unique<Screenshot>());

    m_selected_tool = "Inspect";
}

void Map::init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth) {
    assert(!m_bvh);

    set_minmax_coord(minmax_coords);
    m_max_bvh_depth = max_depth;
    m_render_bvh_depth = max_depth;
    m_bvh = std::make_unique<BVH>(minmax_coords, max_depth, 0);
}

void Map::rebuild_vaos() {
    assert(m_bvh);
    
    mlog::logln(mlog::INFO, "rebuilding vertex arrays...");
    m_bvh->rebuild_vaos();
    mlog::logln(mlog::INFO, "done.");
}

void Map::draw_scene(Viewport& viewport, InputState& input) {
    if(m_heightmap != nullptr)
        m_heightmap->draw_scene(viewport, input);

    auto view_box = viewport.viewport_bbox();

    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);

    auto view = context->get_element<MapView>();
    view->get_theme()->use(*m_shader);

    auto zoom = viewport.get_zoom_factor();
    auto scale = viewport.get_scale_factor();

    if(m_auto_priority)
        m_draw_priority = static_cast<DrawPriority>(std::clamp(int(zoom * 2 + std::sqrt(zoom * 4)), 1, int(DrawPriority::__DRAW_PRIO_LAST)));

    m_bvh->draw(view_box, m_draw_priority, m_render_bvh_depth, 0, scale);

    if(auto tool_name = m_selected_tool) {
        auto &selected_tool = m_tools[*tool_name];
        selected_tool->draw_scene(*this, viewport, input);
    }
}

void Map::menu_item() {
    std::string tools_menu = "Tools";
    if(const auto& tool_name = m_selected_tool) {
        tools_menu += " [" + *tool_name + "]";
    }

    if(ImGui::BeginMenu(tools_menu.c_str())) {
        for(const auto& [name, tool] : m_tools) {
            if(ImGui::MenuItem(name.c_str(), nullptr, m_selected_tool == name, true))
                m_selected_tool = name;
        }
        ImGui::EndMenu();
    }
}

void Map::draw_ui(InputState& input) {
    if(m_heightmap != nullptr)
        m_heightmap->draw_ui(input);

    ImGui::Begin("View");

    auto [min, max] = get_minmax_coord();
    ImGui::Text("coordinate system: (%f, %f) to (%f, %f)", min.x, min.y, max.x, max.y);

    ImGui::Separator();

    ImGui::Checkbox("Auto Priority", &m_auto_priority);

    ImGui::SliderInt("Draw Priority", reinterpret_cast<int*>(&m_draw_priority), __DRAW_PRIORITY_FIRST, __DRAW_PRIO_LAST);

    ImGui::End();


    ImGui::Begin("Tools");

    for(const auto& [name, tool] : m_tools) {
        ImGui::BeginDisabled(name == m_selected_tool);

        if(ImGui::Button(name.c_str()))
            m_selected_tool = name;

        ImGui::EndDisabled();
        ImGui::SameLine();
    }

    ImGui::End();

    if(const auto& tool_name = m_selected_tool) {
        auto &selected_tool = m_tools[*tool_name];
        selected_tool->draw_ui(*this, input);
    }
}

static std::optional<std::string> file_dialog(const nfdchar_t* filter) {
    nfdchar_t *out_path;

    auto cwd = std::filesystem::current_path();

    switch(NFD_OpenDialog(filter, cwd.c_str(), &out_path)) {
        case NFD_OKAY:
            return std::string(out_path);
        case NFD_CANCEL:
            return std::nullopt;
        default:
            mlog::logln(mlog::INFO, "NFD Error: %s", NFD_GetError());
            return std::nullopt;
    }
}

std::expected<std::shared_ptr<Map>, int> load_map(std::string xml_path, std::shared_ptr<Map> map, std::unique_ptr<LoaderContext> context, MapLoader *loader) {
    if(!context->make_current())
        return std::unexpected(EFAULT);

    int err = preprocess_data(xml_path, map, &loader->m_loading_progress);
    context->finalize();

    if(err)
        return std::unexpected(err);
    return map;
}

void MapLoader::menu_item() {
    if(ImGui::BeginMenu("Load")) {
        auto map = context->get_element<Map>();

        ImGui::BeginDisabled(m_loading_map.has_value() || map != nullptr);

        if(ImGui::MenuItem("Map [osm/xml]")) {
            if(auto osm_path = file_dialog("osm;xml")) {
                mlog::logln(mlog::INFO, "Loading OSM Map '%s'...", osm_path->c_str());
                auto map = std::make_shared<Map>();

                if(auto loader_context = context->create_loader_context())
                    m_loading_map = std::async(&load_map, *osm_path, map, std::move(*loader_context), this);
            }
        }

        ImGui::EndDisabled();

        ImGui::BeginDisabled(!map);

        if(ImGui::MenuItem("Tag Info [xml]")) {
            if(auto osm_path = file_dialog("xml")) {
                mlog::logln(mlog::INFO, "Loading Tag-Info '%s'...", osm_path->c_str());
            }
        }

        if(ImGui::MenuItem("Heightmap [tif]")) {
            if(auto osm_path = file_dialog("tif,tiff")) {
                mlog::logln(mlog::INFO, "Loading Heightmap '%s'...", osm_path->c_str());
            }
        }

        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

}

void MapLoader::draw_ui(InputState& input) {
    if(auto& loading = m_loading_map) {
        ImGui::Begin("Loading Map...");
        m_loading_progress.draw_progress_bar();
        ImGui::End();

        if(loading->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            if(auto result = loading->get(); result.has_value()) {
                mlog::logln(mlog::INFO, "loading done!");
                (*result)->rebuild_vaos();
                context->add_map(*result);
            }
            else {
                mlog::logln(mlog::ERROR, "Error loading map: \"%s\"", std::strerror(result.error()));
            }
            m_loading_map = std::nullopt;
        }
    }
}

void MapTheme::use(const Shader& shader) {
    shader.upload_uniform("u_Colormap", m_entries);
}

PresetTheme::PresetTheme(glm::vec4 primary, glm::vec4 secundary, glm::vec4 water, glm::vec4 foliage, std::array<glm::vec4, 3> accent, glm::vec3 background, glm::vec4 trans)
    : MapTheme({
            { Metadata::UNKNOWN,                 secundary},
            { Metadata::HIGHWAY_MOTORWAY,        accent[0] },
            { Metadata::HIGHWAY_TRUNK,           accent[0] },
            { Metadata::HIGHWAY_PRIMARY,         accent[1] },
            { Metadata::HIGHWAY_SECONDARY,       accent[1] },
            { Metadata::HIGHWAY_TERTIARY,        primary   },
            { Metadata::HIGHWAY_UNCLASSIFIED,    primary   },
            { Metadata::HIGHWAY_RESIDENTIAL,     primary   },
            { Metadata::HIGHWAY_LIVING_STREET,   primary   },
            { Metadata::HIGHWAY_SERVICE,         primary   },
            { Metadata::HIGHWAY_PEDESTRIAN,      primary   },
            { Metadata::HIGHWAY_TRACK,           primary   },
            { Metadata::HIGHWAY_BUSWAY,          primary   },
            { Metadata::HIGHWAY_FOOTWAY,         primary   },
            { Metadata::HIGHWAY_CYCLEWAY,        primary   },
            { Metadata::FOOTWAY_SIDEWALK,        primary   },
            { Metadata::FOOTWAY_CROSSING,        primary   },
            { Metadata::RAILWAY,                 accent[2] },
            { Metadata::WATER,                   water     },
            { Metadata::WATERWAY,                water     },
            { Metadata::LANDUSE_AGRICULTURAL,    foliage   },
            { Metadata::LANDUSE_RECREATIONAL,    foliage   },
            { Metadata::LANDUSE_FOREST,          foliage   },
            { Metadata::LANDUSE_INDUSTRIAL,      secundary },
            { Metadata::LANDUSE_COMMERCIAL,      secundary },
            { Metadata::LANDUSE_RESIDENTIAL,     secundary },
            { Metadata::LANDUSE_TRANSPORT,       secundary },
            { Metadata::AERIALWAY_GONDOLA,       accent[2] },
            { Metadata::POWER_LINE,              trans     },
            { Metadata::POWER_DISTRIBUTION,      trans     },
    }, background)
{}


// Red Theme:
/*
const vec4 s_accent_1 = vec4(0.984,0.388,0.463, 1.0);
const vec4 s_accent_2 = vec4(0.365,0.165,0.259, 1.0);
const vec4 s_accent_3 = vec4(0.365,0.165,0.259, 1.0);
const vec4 s_primary = vec4(0.988,0.694,0.651, 1.0);
const vec4 s_water = vec4(0.518,0.863,0.776, 1.0);
const vec4 s_secundary = vec4(1.,0.863,0.8, 1.0);
const vec4 s_foliage = s_trans;
*/

// Purple Theme:
/*
const vec4 s_accent_1 = vec4(0.867,0.067,0.333, 1.0);
const vec4 s_accent_2 = vec4(1.,0.922,0.906, 1.0);
const vec4 s_accent_3 = vec4(1.,0.922,0.906, 1.0);
const vec4 s_primary = vec4(0.624,0.525,0.753, 1.0);
const vec4 s_water = vec4(0.325,0.847,0.984, 1.0);
const vec4 s_secundary = vec4(0.369,0.329,0.557, 1.0);
const vec4 s_foliage = s_trans;
*/

// Grayscale Theme:
/*const vec4 s_accent_1 = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_accent_2 = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_accent_3 = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_primary = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_water = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_secundary = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 s_foliage = s_trans;
*/


/*const vec4 c_Colormap[] = vec4[](
    vec4(0.3, 0.3, 0.3, 0.5), // unknown
    vec4(1.00, 0.32, 0.31, 1.0), // highway motorway
    vec4(1.00, 0.56, 0.31, 1.0), // highway trunk
    vec4(1.00, 0.71, 0.31, 1.0), // highway primary
    vec4(1.00, 0.87, 0.52, 1.0), // highway secondary
    vec4(0.77, 0.77, 0.77, 1.0), // highway tertiary
    vec4(0.70, 0.70, 0.70, 1.0), // highway unclassified
    vec4(0.77, 0.77, 0.77, 1.0), // highway residential
    vec4(0.55, 0.75, 0.89, 1.0), // living street
    vec4(0.33, 0.33, 0.33, 1.0), // service
    vec4(0.33, 0.69, 0.55, 1.0), // pedestrian
    vec4(0.48, 0.40, 0.30, 1.0), // track
    vec4(0.32, 0.34, 0.55, 1.0), // busway
    vec4(0.50, 0.50, 0.50, 1.0), // footway
    vec4(0.50, 0.40, 0.59, 1.0), // cycleway
    vec4(0.50, 0.50, 0.50, 1.0), // footway sidewalk
    vec4(1.0), // footway crossing

    vec4(1.0), // railway
    vec4(0.36, 0.49, 0.89, 1.0), // waterway
    vec4(0.36, 0.49, 0.89, 1.0), // lake

    vec4(0.58, 0.75, 0.41, 1.0), // landuse agricultural
    vec4(0.24, 0.36, 0.22, 1.0), // landuse forest
    vec4(0.89, 0.55, 0.62, 1.0), // landuse industrial
    vec4(0.58, 0.75, 0.41, 1.0), // landuse recreational
    vec4(0.89, 0.55, 0.62, 1.0), // landuse transport
    vec4(0.89, 0.55, 0.62, 1.0), // landuse commercial
    vec4(0.3, 0.3, 0.3, 0.5), // landuse residential
    
    vec4(0.85, 0.28, 0.28, 1.0), // aerialway

    vec4(0.46, 0.18, 0.63, 1.0), // power lines
    vec4(0.46, 0.18, 0.63, 1.0), // power distribution

    vec4(1.0, 0.0, 1.0, 1.0)
);*/

void MapView::load_presets() {
    m_presets["Navy"] = std::make_shared<PresetTheme>(NAVY_THEME);
    m_presets["Sage"] = std::make_shared<PresetTheme>(SAGE_THEME);
    m_presets["Grayscale"] = std::make_shared<PresetTheme>(GRAYSCALE_THEME);

    if(m_presets.find(m_theme) == m_presets.end()) {
        mlog::logln(mlog::ERROR, "invalid theme \"%s\".", m_theme->c_str());
        m_theme = "Sage";
    }
}

void MapView::menu_item() {
    if(ImGui::BeginMenu("View")) {
        ImGui::BeginDisabled(m_presets.find(m_theme) == m_presets.end());

        if(ImGui::MenuItem("Edit Theme")) {
            m_editing = true;            
        }

        ImGui::EndDisabled();

        if(ImGui::BeginMenu("Load Preset")) {
            for(auto [name, _] : m_presets) {
                if(ImGui::MenuItem(name.c_str(), nullptr, m_theme == name))
                    m_theme = name;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

void MapView::draw_ui(InputState& input) {
    if(!m_editing)
        return;

    assert(m_presets.find(m_theme) != m_presets.end());
    auto& theme = m_presets[m_theme];

    ImGui::Begin("Theme Editor", &m_editing);

    ImGui::ColorEdit3("BACKGROUND", reinterpret_cast<float*>(&(theme->background())));

    ImGui::Separator();

    for(size_t i = 0; i < MapTheme::N; i++) {
        auto class_ = Metadata::Classification(i);

        ImGui::ColorEdit4(Metadata::classification_name(class_)->c_str(), reinterpret_cast<float*>(&(**theme)[class_]));
    }

    ImGui::End();
}

