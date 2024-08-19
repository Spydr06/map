#include "inspector.hpp"

#include <imgui.h>

void Inspector::inspect_ui(std::shared_ptr<Way> way) {
    ImGui::Begin("Inspector");

    ImGui::Text("id: %lu", way->get_id());

    ImGui::Separator();

    for(auto& [ key, value ] : way->get_tags()) {
        ImGui::Text("%s := %s", key.c_str(), value.c_str());
    }

    ImGui::End();
}
