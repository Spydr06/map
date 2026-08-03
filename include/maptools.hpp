#pragma once

#include "renderutil.hpp"
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
        static constexpr float EPSILON = 0.00001f;

        inline auto size() -> glm::vec2 const {
            return glm::abs(m_start - m_end);
        }

        inline auto project() -> Range {
            return Range{
                map_project(m_start),
                map_project(m_end)
            };
        }

        inline auto get_coords() -> std::array<glm::vec2, 4> const {
            return std::array<glm::vec2, 4>{
                m_start,
                glm::vec2(m_start.x, m_end.y),
                glm::vec2(m_end.x, m_start.y),
                m_end,
            };
        }

        glm::vec2 m_start;
        glm::vec2 m_end;
    };

    RectangleSelect();
    ~RectangleSelect();

    virtual void draw_ui(Map &map, InputState& input) override;
    virtual void draw_scene(Map &map, Viewport& viewport, InputState& input) override;

private:
    void create_buffers();
    void create_model();
    void take_screenshot();

private:
    bool m_dragging = false;
    float m_viewport_scale = 1.0f;

    std::unique_ptr<Shader> m_selection_shader;
    glm::vec4 m_selection_color;

    std::optional<Range> m_range;

    GLuint m_vbo = 0, m_vao = 0, m_ebo = 0;
};

