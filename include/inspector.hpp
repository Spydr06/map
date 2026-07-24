#pragma once

#include "map.hpp"
#include "maptools.hpp"

class Inspector : public MapTool {
public:
    Inspector();

    virtual void draw_ui(Map &map, InputState& input) override;
    virtual void draw_scene(Map &map, Viewport& viewport, InputState& input) override;

private:
    bool m_fixed = false;

    std::unique_ptr<Shader> m_selection_shader;
    std::shared_ptr<Way> m_selected_way;
};

