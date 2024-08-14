#include "map.hpp"
#include "bvh.hpp"
#include "way.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

#include <imgui.h>

Map::Map()
    : m_bvh(nullptr) 
{
    auto vertex_source = std::ifstream("shaders/map_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/map_fragment.glsl");
    if(vertex_source.bad() || fragment_source.bad()) {
        std::cerr << "Shader error: Shader file not found" << std::endl;
        std::exit(1);
    }

    m_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_shader->get_error()) {
        std::cerr << "Shader error:" << std::endl << *err << std::endl;
        std::exit(1);
    }
}

void Map::init_bvh(std::pair<glm::vec2, glm::vec2> minmax_coords, size_t max_depth) {
    assert(!m_bvh);

    set_minmax_coord(minmax_coords);
    m_max_bvh_depth = max_depth;
    m_render_bvh_depth = max_depth;
    m_bvh = std::make_unique<BVH>(minmax_coords, max_depth, 0);
}

void Map::draw_scene(Viewport& viewport, InputState& input) {
    auto view_box = viewport.viewport_bbox();

    m_shader->use();
    viewport.upload_uniforms(m_shader->id(), input.window_size);

    auto scale = viewport.get_scale_factor();

    int priority = std::clamp(int(scale * 2 + std::sqrt(scale * 4)), 1, int(DrawPriority::__DRAW_PRIO_LAST));

    m_bvh->draw(view_box, static_cast<DrawPriority>(priority), m_render_bvh_depth, 0);
}

void Map::draw_ui(InputState& input) {
    auto [dist, way] = get_nearest_way(input.mapped_cursor_pos);

    if(way != nullptr) {
        ImGui::Begin("Inspector");

        ImGui::Text("id: %lu", way->get_id());

        ImGui::Separator();

        for(auto& [ key, value ] : way->get_tags()) {
            ImGui::Text("%s := %s", key.c_str(), value.c_str());
        }

        ImGui::End();
    }
}

