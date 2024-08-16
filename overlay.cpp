#include "overlay.hpp"
#include "renderutil.hpp"

#include <fstream>
#include <iostream>

Overlay::Overlay() {
    auto vertex_source = std::ifstream("shaders/overlay_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/overlay_fragment.glsl");
    if(vertex_source.bad() || fragment_source.bad()) {
        std::cerr << "Shader error: Shader file not found" << std::endl;
        std::exit(1);
    }

    m_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_shader->get_error()) {
        std::cerr << "Shader error:" << std::endl << *err << std::endl;
        std::exit(1);
    }
}

Overlay::~Overlay() {
}

void Overlay::draw_scene(Viewport& viewport, InputState& input) {
//    auto view_box = viewport.viewport_bbox();

    m_shader->use();
    viewport.upload_uniforms(m_shader->id(), input.window_size);

    auto cursor = input.mapped_cursor_pos;
    glm::vec2 vertices[]{
        cursor,
        cursor + glm::vec2(0.001, 0.001),
        cursor + glm::vec2(-0.001, 0.001)
    };

    // TODO: draw overlay
};

void Overlay::draw_ui(InputState& input) {
};

