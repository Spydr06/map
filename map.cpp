#include "map.hpp"
#include "bvh.hpp"
#include "way.hpp"
#include "log.hpp"

#include <cmath>
#include <fstream>

#include <imgui.h>

Map::Map()
    : m_bvh(nullptr) 
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


    auto sel_vertex_source = std::ifstream("shaders/map_selected_vertex.glsl");
    auto sel_fragment_source = std::ifstream("shaders/map_selected_fragment.glsl");
    if(sel_vertex_source.bad() || sel_fragment_source.bad()) {
        mlog::logln(mlog::ERROR, "Shader error: Shader file not found");
        std::exit(1);
    }

    m_selection_shader = std::make_unique<Shader>(sel_vertex_source, sel_fragment_source);
    if(auto err = m_selection_shader->get_error()) {
        mlog::logln(mlog::ERROR, "Shader error: %s", err->c_str());
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
    viewport.upload_uniforms(*m_shader, input.window_size);

    auto scale = viewport.get_scale_factor();

    m_draw_priority = static_cast<DrawPriority>(std::clamp(int(scale * 2 + std::sqrt(scale * 4)), 1, int(DrawPriority::__DRAW_PRIO_LAST)));

    m_bvh->draw(view_box, m_draw_priority, m_render_bvh_depth, 0);

    if(m_selected_way) {
        m_selection_shader->use();
        m_selection_shader->upload_uniform("u_Resolution", input.window_size);
        viewport.upload_uniforms(*m_selection_shader, input.window_size);

        m_selected_way->draw_highlighted_buffers();
    }
}

void Map::draw_ui(InputState& input) {
    auto [dist, way] = get_nearest_way(input.mapped_cursor_pos);
    m_selected_way = way;
    
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

