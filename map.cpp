#include "map.hpp"
#include "main.hpp"
#include "bvh.hpp"
#include "inspector.hpp"
#include "preprocess.hpp"
#include "screenshot.hpp"
#include "way.hpp"
#include "log.hpp"
#include "renderutil.hpp"

#include <cmath>
#include <expected>
#include <filesystem>
#include <fstream>

#include <future>
#include <imgui.h>
#include <memory>
#include <nfd.h>

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

void Map::draw_scene(Viewport& viewport, InputState& input) {
    if(m_heightmap != nullptr)
        m_heightmap->draw_scene(viewport, input);

    auto view_box = viewport.viewport_bbox();

    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);

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

std::expected<std::shared_ptr<Map>, int> load_map(std::string xml_path, std::shared_ptr<Map> map, MapLoader *loader) {
    if(int err = preprocess_data(xml_path, map, &loader->m_loading_map_progress)) {
        return std::unexpected(err);
    }

    return map;
}

void MapLoader::menu_item() {
    if(ImGui::BeginMenu("Load")) {
        ImGui::BeginDisabled(m_loading_map.has_value());

        auto map = context->get_element<Map>();

        if(ImGui::MenuItem("Map [osm/xml]")) {
            if(auto osm_path = file_dialog("osm;xml")) {
                mlog::logln(mlog::INFO, "Loading OSM Map '%s'...", osm_path->c_str());
                auto map = std::make_shared<Map>();

                m_loading_map = std::async(&load_map, *osm_path, map, this);
            }
        }

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
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

}

void MapLoader::draw_ui(InputState& input) {
    if(auto& loading = m_loading_map) {
        ImGui::Begin("Loading Map...");
        ImGui::ProgressBar(m_loading_map_progress.load() / 1024.0f / 1024.0f, ImVec2(0.0f, 0.0f), "(MiB)");
        ImGui::End();
    }
}

