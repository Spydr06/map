#pragma once

#include "viewport.hpp"
#include "inputstate.hpp"

#include <glm/glm.hpp>
#include <memory>

class Map;

class MapTool {
public:
    MapTool() 
    {}

    MapTool(MapTool&) = default;
    virtual ~MapTool() = default;

    virtual void draw_scene(Map &map, Viewport& viewport, InputState& input) = 0;
    virtual void draw_ui(Map &map, InputState& input) = 0;
};

class RectangleSelect : public MapTool {
public:
    struct Range {
    public:
        Range() {
            
        }

        inline auto size() -> glm::vec2 const {
            return glm::abs(m_max - m_min);
        }

        glm::vec2 m_min;
        glm::vec2 m_max;
    };

    RectangleSelect();

    virtual void draw_ui(Map &map, InputState& input) override;
    virtual void draw_scene(Map &map, Viewport& viewport, InputState& input) override;

private:
    bool m_dragging = false;

    std::unique_ptr<Shader> m_selection_shader;
    std::optional<Range> m_range;
};

