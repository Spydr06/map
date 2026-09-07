#include "inspector.hpp"
#include "heightmap.hpp"
#include "main.hpp"
#include "way.hpp"

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
    ImGui::SetNextItemShortcut(ImGuiKey_Escape);
    if(ImGui::Button("Clear (ESC)"))
        m_fixed = false;
    else if(!m_fixed)
        ImGui::EndDisabled();

    if(m_selected_way)
        m_selected_way->inspect(ImGuiTreeNodeFlags_Framed);

    ImGui::End();
}

void MapElement::inspect(ImGuiTreeNodeFlags flags) const {
    if(ImGui::TreeNodeEx("Attributes", ImGuiTreeNodeFlags_DefaultOpen | flags)) {
        if(ImGui::BeginTable("##inspect-attrs", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("id");
            ImGui::TableNextColumn();
            ImGui::Text("%lu", get_id());

            for(auto& [ key, value ] : get_tags()) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%s", key.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", value.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::TreePop();
    }

    if(ImGui::TreeNodeEx("Tag Information", flags)) {
        const auto map = context->get_element<Map>();

        for(const auto& [ key, value ] : get_tags()) {
            for(auto* tag : map->get_taginfos(key, value)) {
                tag->load_image();
                ImVec2 size(tag->m_dimensions.x, tag->m_dimensions.y);
                ImGui::Image((void*)(uintptr_t) tag->m_texture_id, size);
            }
        }

        ImGui::TreePop();
    }
}

void Way::inspect(ImGuiTreeNodeFlags flags) const {
    MapElement::inspect(flags);

    if(ImGui::TreeNodeEx("Altitude Graph", ImGuiTreeNodeFlags_DefaultOpen | flags)) {
        if(const auto heightmap = context->get_element<Heightmap>()) {
            const auto points = heightmap->altitude_graph(*this);
            if(points.empty())
                ImGui::Text("Selection outside of heightmap.");
            else {
                ImGui::PlotLines("Altitude", points.data(), points.size(), 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0.0f, 96.0f));

                const auto [min, max] = std::minmax_element(points.begin(), points.end());
                ImGui::Text("Minimum: %.1f [m]", *min);
                ImGui::Text("Maximum: %.1f [m]", *max);
            }
        }
        else {
            ImGui::Text("No heightmap loaded.");
        }

        ImGui::TreePop();
    }
}

void Relation::inspect(ImGuiTreeNodeFlags flags) const {
    MapElement::inspect(flags);

    if(ImGui::TreeNodeEx("Referenced Ways", flags)) {
        if(auto highlight = context->get_element<MapHighlight>()) {
            auto map = context->get_element<Map>();
            assert(map != nullptr);

            if(ImGui::Button("Hightlight Ways")) {
                for(auto& way : *map)  {
                    if(m_ways.contains(way->get_id()))
                        highlight->add_way(way);
                }
            }
        }

        for(const auto ref : m_ways) {
            ImGui::Text("%lu", ref);
        }
        
        ImGui::TreePop();
    }
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

