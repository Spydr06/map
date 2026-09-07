#include "map.hpp"
#include "inspector.hpp"
#include "preprocess.hpp"
#include "rendercontext.hpp"
#include "screenshot.hpp"
#include "taginfo.hpp"
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

static const MapTheme CLASSIC_THEME(
    {
        {Metadata::UNKNOWN, glm::vec4(0.3, 0.3, 0.3, 0.5)}, // unknown
        {Metadata::HIGHWAY_MOTORWAY, glm::vec4(1.00, 0.32, 0.31, 1.0)}, // highway motorway
        {Metadata::HIGHWAY_TRUNK, glm::vec4(1.00, 0.56, 0.31, 1.0)}, // highway trunk
        {Metadata::HIGHWAY_PRIMARY, glm::vec4(1.00, 0.71, 0.31, 1.0)}, // highway primary
        {Metadata::HIGHWAY_SECONDARY, glm::vec4(1.00, 0.87, 0.52, 1.0)}, // highway secondary
        {Metadata::HIGHWAY_TERTIARY, glm::vec4(0.77, 0.77, 0.77, 1.0)}, // highway tertiary
        {Metadata::HIGHWAY_UNCLASSIFIED, glm::vec4(0.70, 0.70, 0.70, 1.0)}, // highway unclassified
        {Metadata::HIGHWAY_RESIDENTIAL, glm::vec4(0.77, 0.77, 0.77, 1.0)}, // highway residential
        {Metadata::HIGHWAY_LIVING_STREET, glm::vec4(0.55, 0.75, 0.89, 1.0)}, // living street
        {Metadata::HIGHWAY_SERVICE, glm::vec4(0.33, 0.33, 0.33, 1.0)}, // service
        {Metadata::HIGHWAY_PEDESTRIAN, glm::vec4(0.33, 0.69, 0.55, 1.0)}, // pedestrian
        {Metadata::HIGHWAY_TRACK, glm::vec4(0.48, 0.40, 0.30, 1.0)}, // track
        {Metadata::HIGHWAY_BUSWAY, glm::vec4(0.32, 0.34, 0.55, 1.0)}, // busway
        {Metadata::HIGHWAY_FOOTWAY, glm::vec4(0.50, 0.50, 0.50, 1.0)}, // footway
        {Metadata::HIGHWAY_CYCLEWAY, glm::vec4(0.50, 0.40, 0.59, 1.0)}, // cycleway
        {Metadata::FOOTWAY_SIDEWALK, glm::vec4(0.50, 0.50, 0.50, 1.0)}, // footway sidewalk
        {Metadata::FOOTWAY_CROSSING, glm::vec4(1.0)}, // footway crossing

        {Metadata::RAILWAY, glm::vec4(1.0)}, // railway
        {Metadata::WATERWAY, glm::vec4(0.36, 0.49, 0.89, 1.0)}, // waterway
        {Metadata::WATER, glm::vec4(0.36, 0.49, 0.89, 1.0)}, // lake

        {Metadata::LANDUSE_AGRICULTURAL, glm::vec4(0.58, 0.75, 0.41, 1.0)}, // landuse agricultural
        {Metadata::LANDUSE_FOREST, glm::vec4(0.24, 0.36, 0.22, 1.0)}, // landuse forest
        {Metadata::LANDUSE_INDUSTRIAL, glm::vec4(0.89, 0.55, 0.62, 1.0)}, // landuse industrial
        {Metadata::LANDUSE_RECREATIONAL, glm::vec4(0.58, 0.75, 0.41, 1.0)}, // landuse recreational
        {Metadata::LANDUSE_TRANSPORT, glm::vec4(0.89, 0.55, 0.62, 1.0)}, // landuse transport
        {Metadata::LANDUSE_COMMERCIAL, glm::vec4(0.89, 0.55, 0.62, 1.0)}, // landuse commercial
        {Metadata::LANDUSE_RESIDENTIAL, glm::vec4(0.3, 0.3, 0.3, 0.5)}, // landuse residential

        {Metadata::AERIALWAY_GONDOLA, glm::vec4(0.85, 0.28, 0.28, 1.0)}, // aerialway

        {Metadata::POWER_LINE, glm::vec4(0.46, 0.18, 0.63, 1.0)}, // power lines
        {Metadata::POWER_DISTRIBUTION, glm::vec4(0.46, 0.18, 0.63, 1.0)}, // power distribution
    },
    glm::vec3(0.0)
);


void Progress::draw_progress_bar() const {
    auto total = m_total.load();
    auto progress = m_progress.load();
    auto frac = progress / total; 

    std::string s = std::format("{:.1f}%", frac * 100.0f);
    ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), s.c_str());

    ImGui::Text("%.1f of %.1f %s", progress, total, m_unit.c_str());
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

void Map::on_attach(RenderContext& context) {
    context.center_viewport(*this);
    context.add_element(std::make_shared<MapHighlight>());
}

void Map::draw_scene(Viewport& viewport, InputState& input) {
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
    ImGui::Begin("View");

    auto [min, max] = get_minmax_coord();
    ImGui::Text("coordinate system: (%f, %f) to (%f, %f)", min.x, min.y, max.x, max.y);

    if(ImGui::Button("Center Viewport")) {
        context->center_viewport(*this);
    }

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

        auto menu_item = [](const char *name, const std::optional<std::string>& path) {
            if(auto p = path)
                return ImGui::MenuItem(std::format("{} - \"{}\"", name, *p).c_str());
            return ImGui::MenuItem(name);
        };

        auto path_basename = [](std::string path) {
            if(auto start = path.rfind('/'); start != std::string::npos)
                path.erase(0, start + 1);
            return path;
        };

        if(menu_item("Map [osm/xml]", m_map_path)) {
            if(auto osm_path = file_dialog("osm;xml")) {
                mlog::logln(mlog::INFO, "Loading OSM Map '%s'...", osm_path->c_str());
                auto map = std::make_shared<Map>();

                if(auto loader_context = context->create_loader_context())
                    m_loading_map = std::async(&load_map, *osm_path, map, std::move(*loader_context), this);
                m_map_path = path_basename(std::move(*osm_path));
            }
        }

        ImGui::EndDisabled();

        ImGui::BeginDisabled(!map);

        if(menu_item("Tag Info [xml]", m_taginfo_path)) {
            if(auto taginfo_path = file_dialog("xml")) {
                mlog::logln(mlog::INFO, "Loading Tag-Info '%s'...", taginfo_path->c_str());

                if(int err = load_taginfo(taginfo_path->c_str(), map))
                    mlog::logln(mlog::ERROR, "Could not load tag information from '%s': %s", taginfo_path->c_str(), std::strerror(err));
                else
                    m_taginfo_path = path_basename(std::move(*taginfo_path));
            }
        }

        ImGui::EndDisabled();

        bool heightmap_loaded = context->get_element<Heightmap>() != nullptr;
        ImGui::BeginDisabled(heightmap_loaded);

        if(m_heightmap_path && heightmap_loaded)
            m_heightmap_path = std::nullopt;

        if(menu_item("Heightmap [tif]", m_heightmap_path)) {
            if(auto hm_path = file_dialog("tif,tiff")) {
                mlog::logln(mlog::INFO, "Loading Heightmap '%s'...", hm_path->c_str());

                auto heightmap = std::make_shared<Heightmap>(*hm_path);
                if(int err = heightmap->preprocess())
                    mlog::logln(mlog::ERROR, "Could not load heightmap from \"%s\": %s", hm_path->c_str(), std::strerror(err));
                else {
                    context->add_element(heightmap);
                    m_heightmap_path = path_basename(std::move(*hm_path));
                }
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
                context->add_element(*result);
                context->center_viewport(**result);
            }
            else {
                mlog::logln(mlog::ERROR, "Error loading map: \"%s\"", std::strerror(result.error()));
                m_map_path = std::nullopt;
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

void MapView::load_presets() {
    m_presets["Navy"] = std::make_shared<PresetTheme>(NAVY_THEME);
    m_presets["Sage"] = std::make_shared<PresetTheme>(SAGE_THEME);
    m_presets["Grayscale"] = std::make_shared<PresetTheme>(GRAYSCALE_THEME);
    m_presets["Classic"] = std::make_shared<MapTheme>(CLASSIC_THEME);

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

MapHighlight::MapHighlight()
    : m_ways{}, m_color{1.0f, 0.3f, 0.3f, 1.0f}, m_weight{2.0f}, m_show{true}
{
    auto vertex_source = std::ifstream("shaders/map_highlight_vertex.glsl");
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
}

void MapHighlight::add_way(std::shared_ptr<Way> way) {
    m_ways[way->get_id()] = way;
}

void MapHighlight::draw_scene(Viewport& viewport, InputState& input) {
    if(m_show && !m_ways.empty()) {
        m_shader->use();
        m_shader->upload_uniform("u_Color", *m_color);
        viewport.upload_uniforms(*m_shader, input.window_size);

        auto view = context->get_element<MapView>();
        view->get_theme()->use(*m_shader);

        auto scale = viewport.get_scale_factor();

        for(auto [_, way] : m_ways) {
            way->draw_buffers(scale * m_weight);
        }
    }
}

void MapHighlight::draw_ui(InputState& input) {
    ImGui::Begin("Highlight");

    bool show = m_show;
    if(ImGui::Checkbox("Show Highlights##hl", &show))
        m_show = !m_show;

    glm::vec4 color = m_color;
    ImGui::ColorEdit4("Color##hl", reinterpret_cast<float*>(&color));
    if(color != m_color)
        m_color = color;

    float weight = m_weight;
    ImGui::SliderFloat("Weight##hl", &weight, 1.0f, 10.0f);
    if(weight != m_weight)
        m_weight = weight;

    if(ImGui::Button("Remove All##hl")) {
        m_ways.clear();
    }

    ImGui::Separator();

    if(m_ways.empty()) {
        ImGui::Text("No ways highlighted.");
    }
    else {
        std::optional<Way::Id> remove = std::nullopt;
        for(const auto& [id, way] : m_ways) {
            ImGui::PushID(static_cast<void*>(way.get()));

            auto& tags = way->get_tags();
            bool node;
            if(tags.contains("name"))
                node = ImGui::TreeNode("##hl-way", "%s: %lu, \"%s\"", way->element_type().c_str(), way->get_id(), tags["name"].c_str());
            else
                node = ImGui::TreeNode("##hl-way", "%s: %lu", way->element_type().c_str(), way->get_id());

            if(node) {
                if(ImGui::Button("Remove"))
                    remove = id;

                way->inspect();
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if(remove != std::nullopt)
            m_ways.erase(*remove);
    }

    ImGui::End();
}

