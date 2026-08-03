#include "maptools.hpp"

#include "main.hpp"
#include "map.hpp"
#include "modelview.hpp"
#include "renderutil.hpp"
#include "screenshot.hpp"
#include "viewport.hpp"

#include <imgui.h>

#include <fstream>
#include <optional>

static constexpr std::array<GLuint, 6> selection_indices = {
    0, 1, 2, 2, 1, 3
};

RectangleSelect::RectangleSelect() 
    : m_selection_color(0.0, 1.0, 1.0, 0.5)
{
    auto vertex_source = std::ifstream("shaders/area_selected_vertex.glsl");
    auto fragment_source = std::ifstream("shaders/area_selected_fragment.glsl");
    if(vertex_source.bad() || fragment_source.bad()) {
        mlog::logln(mlog::ERROR, "Shader error: Shader file not found");
        std::exit(1);
    }

    m_selection_shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if(auto err = m_selection_shader->get_error()) {
        mlog::logln(mlog::ERROR, "Shader error: %s", err->c_str());
        std::exit(1);
    }
}

RectangleSelect::~RectangleSelect() {
    if(m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if(m_vao)
        glDeleteVertexArrays(1, &m_vao);
}

void RectangleSelect::draw_ui(Map& map, InputState& input) {
    ImGui::Begin("Selection");

    if(!m_dragging && input.rmb_down) {
        m_dragging = true;
    
        m_range = Range{
            input.mapped_cursor_pos,
            input.mapped_cursor_pos
        };
    }

    if(m_dragging) {
        m_range->m_end = input.mapped_cursor_pos;
        if(!(m_dragging = input.rmb_down) && m_range->size().x < Range::EPSILON && m_range->size().y < Range::EPSILON)
            m_range = std::nullopt;
    }

    if(auto& range = m_range) {
        float sensitivity = 0.0001f / m_viewport_scale;

        ImGui::SeparatorText("Selection");
        ImGui::Text("Size: (%f, %f)", range->size().x, range->size().y);
        ImGui::DragFloat2("Start", reinterpret_cast<float*>(&range->m_start), sensitivity);
        ImGui::DragFloat2("End", reinterpret_cast<float*>(&range->m_end), sensitivity);

        ImGui::SeparatorText("Actions");
        if(ImGui::Button("Clear"))
            m_range = std::nullopt;

        ImGui::SameLine();
        if(ImGui::Button("Screenshot"))
            take_screenshot();

        ImGui::SameLine();
        if(ImGui::Button("Create Model"))
            create_model();
    }

    ImGui::Separator();

    ImGui::ColorEdit4("Selection Color", reinterpret_cast<float*>(&m_selection_color));

    ImGui::End();
}

void RectangleSelect::create_buffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    assert(m_vao != 0);
    assert(m_vbo != 0);
    assert(m_ebo != 0);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, selection_indices.size() * sizeof(GLuint), selection_indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    auto coords = m_range->get_coords();
    glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(glm::vec2), coords.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void RectangleSelect::draw_scene(Map& map, Viewport& viewport, InputState& input) {
    m_viewport_scale = viewport.get_scale_factor();

    if(auto range = m_range) {
        if(!m_vao) {
            create_buffers();
            assert(m_vao != 0);
        }

        auto selection = range->get_coords();

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, selection.size() * sizeof(glm::vec2), selection.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        m_selection_shader->use();
        viewport.upload_uniforms(*m_selection_shader, input.window_size);

        m_selection_shader->upload_uniform("u_SelColor", m_selection_color);

        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, selection_indices.size(), GL_UNSIGNED_INT, nullptr);
    }
}

void RectangleSelect::take_screenshot() {
    if(!context->get_element<Screenshot>()) {
        context->add_element(std::make_shared<Screenshot>(*m_range));
    }
}

void RectangleSelect::create_model() {
//    context->add_element(std::make_shared<ModelView>(std::make_shared<Model>()));
}

