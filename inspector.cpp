#include "inspector.hpp"

#include <imgui.h>
#include <map.hpp>

#include <fstream>

Inspector::Inspector() 
    : MapTool() {
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

void Inspector::draw_ui(Map& map, InputState& input) {
    if(!m_fixed) {
        auto [dist, way] = map.get_nearest_way(input.mapped_cursor_pos);
        m_selected_way = way;

        if(input.rmb_down)
            m_fixed = true;
    }

    ImGui::Begin("Inspector");

    if(m_fixed)
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Selection Fixed");
    else {
        ImGui::BeginDisabled();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Fix Selection (right-click)");
    }

    ImGui::SameLine();
    if(ImGui::Button("Clear"))
        m_fixed = false;
    else if(!m_fixed)
        ImGui::EndDisabled();

    if(m_selected_way) {
        ImGui::Text("id: %lu", m_selected_way->get_id());

        ImGui::Separator();

        for(auto& [ key, value ] : m_selected_way->get_tags()) {
            ImGui::Text("%s := %s", key.c_str(), value.c_str());
        }

        ImGui::Separator();

        for(auto& [ key, value ] : m_selected_way->get_tags()) {
            for(auto* tag : map.get_taginfos(key, value)) {
                tag->load_image();
                ImVec2 size(tag->m_dimensions.x, tag->m_dimensions.y);
                ImGui::Image((void*)(uintptr_t) tag->m_texture_id, size);
            }
        }
    }

    ImGui::End();
}

void Inspector::draw_scene(Map&, Viewport& viewport, InputState& input) {
    if(!m_selected_way)
        return;

    m_selection_shader->use();
    m_selection_shader->upload_uniform("u_Resolution", input.window_size);
    m_selection_shader->upload_uniform("u_Fixed", m_fixed);

    viewport.upload_uniforms(*m_selection_shader, input.window_size);

    m_selected_way->draw_highlighted_buffers(viewport.get_scale_factor());
}

