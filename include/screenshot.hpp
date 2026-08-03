#pragma once

#include "inputstate.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "viewport.hpp"
#include "maptools.hpp"

#include <filesystem>

#define RESOLUTION_FHD glm::ivec2(1920, 1080)
#define RESOLUTION_4K glm::ivec2(3840, 2160)
#define RESOLUTION_8K glm::ivec2(7680, 4320)

class Screenshot : public RenderElement, public MapTool {
public:
    Screenshot() 
        : m_directory(std::filesystem::current_path())
    {}

    Screenshot(const RectangleSelect::Range& range)
        : m_directory(std::filesystem::current_path()), m_range(range)
    {}

    virtual void draw_scene(Viewport &, InputState&) override {};

    virtual void draw_scene(Map&, Viewport& viewport, InputState& input_state) override {
        draw_scene(viewport, input_state);
    };

    virtual void draw_ui(InputState &input) override;

    virtual void draw_ui(Map& map, InputState& input_state) override;

    virtual bool remove() const override {
        return m_remove;
    }

    void take_screenshot(RenderContext& context);

    inline bool pending() const {
        return m_pending;
    }

    virtual int get_z_index() const override {
        return 10;
    };
private:
    std::filesystem::path m_directory;

    std::optional<RectangleSelect::Range> m_range;

    glm::ivec2 m_resolution = RESOLUTION_4K;
    float m_scale = 1.0f;

    bool m_pending = false;
    bool m_remove = false;
};

