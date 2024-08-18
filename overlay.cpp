#include "overlay.hpp"
#include "renderutil.hpp"
#include "log.hpp"

#include <fstream>

Overlay::Overlay() {
    auto vertex_source = std::ifstream("shaders/overlay_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/overlay_fragment.glsl");
    if(vertex_source.bad() || fragment_source.bad()) {
        mlog::logln(mlog::ERROR, "Shader error: Shader file not found");
        std::exit(1);
    }

    m_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_shader->get_error()) {
        mlog::logln(mlog::ERROR, "Shader error: %s", err->c_str());
        std::exit(1);
    }
}

Overlay::~Overlay() {
}

void Overlay::draw_scene(Viewport& viewport, InputState& input) {
//    auto view_box = viewport.viewport_bbox();

    m_shader->use();
    viewport.upload_uniforms(*m_shader, input.window_size);

    // TODO: draw overlay
};

void Overlay::draw_ui(InputState& input) {
};

