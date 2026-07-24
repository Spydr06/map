#include "maptools.hpp"

#include "map.hpp"

#include <imgui.h>

RectangleSelect::RectangleSelect() {

}

void RectangleSelect::draw_ui(Map& map, InputState& input) {
    ImGui::Begin("Selection");

    if(!m_dragging && input.rmb_down) {
        m_dragging = true;
    
        m_range = Range{
            input.mapped_cursor_pos,
            input.mapped_cursor_pos
        };
    }

    if(m_dragging) {
        m_range->m_max = input.mapped_cursor_pos;
        m_dragging = input.rmb_down;
    }

    if(auto range = m_range) {
        ImGui::Text("Selection: (%f, %f) to (%f, %f)", range->m_min.x, range->m_min.y, range->m_max.x, range->m_max.y);
        ImGui::Text("Size: (%f, %f)", range->size().x, range->size().y);
    }

    ImGui::End();
}

void RectangleSelect::draw_scene(Map& map, Viewport& viewport, InputState& input) {
}

