#include "inspector.hpp"

#include <imgui.h>
#include <cstring>
#include <map.hpp>

void Inspector::inspect_ui(Map* map, std::shared_ptr<Way> way) {
    ImGui::Begin("Inspector");

    ImGui::Text("id: %lu", way->get_id());

    ImGui::Separator();

    for(auto& [ key, value ] : way->get_tags()) {
        ImGui::Text("%s := %s", key.c_str(), value.c_str());
    }

    ImGui::Separator();

    for(auto& [ key, value ] : way->get_tags()) {
        for(auto* tag : map->get_taginfos(key, value)) {
            ImVec2 size(tag->m_dimensions.x, tag->m_dimensions.y);
            ImGui::Image((void*)(uintptr_t) tag->m_texture_id, size);
        }
    }

    ImGui::End();
}
