#pragma once

#include "inputstate.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "viewport.hpp"

#include <filesystem>

#define RESOLUTION_FHD glm::ivec2(1920, 1080)
#define RESOLUTION_4K glm::ivec2(3840, 2160)
#define RESOLUTION_8K glm::ivec2(7680, 4320)

class Screenshot : public RenderElement {
public:
    Screenshot() 
        : m_directory(std::filesystem::current_path())
    {}

    virtual void draw_scene(Viewport &, InputState&) override {};
    virtual void draw_ui(InputState &input) override;

    void take_screenshot(RenderContext& context);

    inline bool pending() const {
        return m_pending;
    }
private:
    std::filesystem::path m_directory;

    glm::ivec2 m_resolution = RESOLUTION_4K;
    float m_scale = 1.0f;
    bool m_pending = false;
};

