#include "map.hpp"
#include "bvh.hpp"
#include "inspector.hpp"
#include "way.hpp"
#include "log.hpp"
#include "renderutil.hpp"

#include <cmath>
#include <fstream>

#include <imgui.h>
#include <memory>

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

void Map::draw_ui(InputState& input) {
    if(m_heightmap != nullptr)
        m_heightmap->draw_ui(input);

    ImGui::Begin("Tools");

    for(const auto& [name, tool] : m_tools) {
        ImGui::BeginDisabled(name == m_selected_tool);

        if(ImGui::Button(name.c_str()))
            m_selected_tool = name;

        ImGui::EndDisabled();
        ImGui::SameLine();
    }

    ImGui::End();

    if(auto tool_name = m_selected_tool) {
        auto &selected_tool = m_tools[*tool_name];
        selected_tool->draw_ui(*this, input);
    }
}

