#include "inspector.hpp"

#include <imgui.h>
#include "log.hpp"
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
        auto& tag = map->m_taginfo[key][value];
        if(tag.m_icon_url.length() == 0) {
            tag = map->m_taginfo[key][value + ".1"];
            if(tag.m_icon_url.length() == 0) {
                if(key == "traffic_sign")
                    mlog::logln(mlog::WARN, "No source for sign %s.", value.c_str());
                continue;
            }
        }

        if(int err = tag.load_image())
            mlog::logln(mlog::ERROR, "Could not get icon from `%s`: %s.", tag.m_icon_url.c_str(), std::strerror(err));

        ImVec2 size(128, 128);
        ImGui::Image((void*)(uintptr_t) tag.m_texture_id, size);
    }

    ImGui::End();
}
