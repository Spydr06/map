#include "query.hpp"
#include "main.hpp"

#include <imgui.h>
#include <iostream>
#include <optional>
#include <regex>

void MapQuery::draw_scene(Viewport& viewport, InputState& input) {
    collect();
}

void MapQuery::draw_ui(InputState& input) {
    if(!m_show_query || !ImGui::Begin("Map Search", &m_show_query))
        return;

    uint64_t limit;

    auto map = context->get_element<Map>();
    if(map == nullptr) {
        ImGui::Text("No map loaded.");
        goto finish_query;
    }

    ImGui::BeginDisabled(m_current_query != std::nullopt);

    ImGui::BeginTable("##filter-table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg);
    ImGui::TableSetupColumn("Attribute");
    ImGui::TableSetupColumn("Key");
    ImGui::TableSetupColumn("Mode");
    ImGui::TableHeadersRow();

    for(auto i = static_cast<int64_t>(m_filters.size()) - 1; i >= 0; i--) {
        auto& filter = m_filters[i];

        ImGui::TableNextRow();
        filter.draw_static();

        ImGui::TableNextColumn();
        ImGui::PushID(&filter);
        if(ImGui::Button("-"))
            m_filters.erase(m_filters.begin() + i);
        ImGui::PopID();
    }

    ImGui::EndTable();

    ImGui::SeparatorText("New Filter");

    m_new_filter.draw_editable();

    ImGui::BeginDisabled(m_new_filter.m_attribute.size() == 0);

    if(ImGui::Button("Add Filter")) {
        m_filters.push_back(std::move(m_new_filter));
        m_new_filter = Filter();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(m_filters.empty());

    if(ImGui::Button("Clear Filters")) {
        m_filters.clear();
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    limit = m_limit;
    ImGui::InputScalar("Item Limit", ImGuiDataType_U64, &limit);
    if(m_limit != limit)
        m_limit = limit;
    
    if(ImGui::Button("Search Map Items...")) {
        query(map);
    }

    ImGui::EndDisabled();

finish_query:
    ImGui::End();

    if(!m_last_query)
        return;

    ImGui::Begin("Search Results");

    const auto& results = *m_last_query;

    if(results.empty()) {
        ImGui::Text("No search results.");
        goto finish_results;
    }

    for(size_t i = 0; i < results.size(); i++) {
        const auto& way = results[i];

        auto& tags = way->get_tags();
        bool node;
        if(tags.contains("name"))
            node = ImGui::TreeNode(std::format("##query-result-{}", i).c_str(), "%s: %lu, \"%s\"", way->element_type().c_str(), way->get_id(), tags["name"].c_str());
        else
            node = ImGui::TreeNode(std::format("##query-result-{}", i).c_str(), "%s: %lu", way->element_type().c_str(), way->get_id());

        if(node) {
            way->inspect();
            ImGui::TreePop();
        }
    }

finish_results:
    ImGui::End();
}

void MapQuery::menu_item() {
    if(ImGui::BeginMenu("Find")) {
        const auto map = context->get_element<Map>();
        ImGui::BeginDisabled(map == nullptr);

        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
        if(ImGui::MenuItem("Search in Map")) {
            m_show_query = true;
        }

        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}

static std::vector<std::shared_ptr<MapElement>> perform_query(std::shared_ptr<Map> map, const std::vector<MapQuery::Filter>& filters, size_t limit) {
    std::vector<std::shared_ptr<MapElement>> result{};
    result.reserve(std::min(limit, MapQuery::QUERY_LIMIT));

    try {
        for(const auto& [_, rel] : map->get_relations()) {
            for(const auto& filter : filters) {
                if(!filter.match(*rel))
                    goto next_rel;
            }

            result.push_back(rel);
            if(result.size() >= limit) {
                std::cout << "limit reached" << std::endl;
                break;
            }
next_rel:
        }

        for(const auto& way : *map) {
            for(const auto& filter : filters) {
                if(!filter.match(*way))
                    goto next_way;
            }

            result.push_back(way);
            if(result.size() >= limit) {
                std::cout << "limit reached" << std::endl;
                break;
            }
next_way:
        }
    } catch(std::regex_error& e) {
        mlog::logln(mlog::ERROR, "regex error: %s", e.what());
        return {};
    }

    return result;
}

void MapQuery::query(std::shared_ptr<Map> map) {
    if(m_current_query)
        return;

    mlog::logln(mlog::INFO, "Begin map query with %zu filters...", m_filters.size());
    m_current_query = std::async(&perform_query, map, m_filters, m_limit);
}

void MapQuery::collect() {
    auto& current = m_current_query; 
    if(current == std::nullopt)
        return;

    if(current->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    m_last_query = current->get();
    m_current_query = std::nullopt;

    mlog::logln(mlog::INFO, "done.");
}

bool MapQuery::Filter::match(MapElement& elem) const {
    const auto& tags = elem.get_tags();

    switch(m_mode) {
        case MapQuery::Filter::FILTER_EXACT:
            if(!tags.contains(m_attribute) || tags.at(m_attribute) != m_filter)
                return false;
            break;
        case MapQuery::Filter::FILTER_NOT:
            if(!tags.contains(m_attribute) || tags.at(m_attribute) != m_filter)
                return false;
            break;
        case MapQuery::Filter::FILTER_SUBSTRING:
            if(!tags.contains(m_attribute))
                return false;

            if(!tags.at(m_attribute).contains(m_filter))
                return false;
            break;
        case MapQuery::Filter::FILTER_REGEX:
            if(!tags.contains(m_attribute))
                return false;

            if(!std::regex_match(tags.at(m_attribute), std::regex(m_filter)))
                return false;
            break;
    }

    return true;
}

void MapQuery::Filter::draw_static() {
    ImGui::PushID(this);

    ImGui::TableNextColumn();

    ImGui::Text("%s", m_attribute.c_str());

    ImGui::TableNextColumn();

    ImGui::Text("%s", m_filter.c_str());

    ImGui::TableNextColumn();

    draw_mode_dropdown();

    ImGui::PopID();
}

void MapQuery::Filter::draw_editable() {
    ImGui::PushID(this);

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    m_attribute.copy(buf, sizeof(buf) - 1);
    if(ImGui::InputText("Attribute", buf, sizeof(buf) - 1)) {
        m_attribute = buf;
    }

    memset(buf, 0, sizeof(buf));
    m_filter.copy(buf, sizeof(buf) - 1);
    if(ImGui::InputText("Key", buf, sizeof(buf) - 1)) {
        m_filter = buf;
    }

    draw_mode_dropdown();
    
    ImGui::SameLine();

    ImGui::Text("Mode");

    ImGui::PopID();
}

void MapQuery::Filter::draw_mode_dropdown() {
    static constexpr const char* MODE_STRINGS[] = {
        "==",
        "!=",
        "{}",
        ".*"
    };

    static constexpr const char* MODE_EXPLANATION[] = {
        "== (match exact string)",
        "!= (negated match)",
        "{} (substring)",
        ".* (match substring)"
    };

    if(ImGui::BeginCombo("##mode", MODE_STRINGS[m_mode])) {
        for(auto mode : {FILTER_EXACT, FILTER_NOT, FILTER_SUBSTRING, FILTER_REGEX}) {
            if(ImGui::Selectable(MODE_EXPLANATION[mode], mode == m_mode)) {
                m_mode = mode;
            }
        }

        ImGui::EndCombo();
    }
}
