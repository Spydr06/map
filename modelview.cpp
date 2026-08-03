#include "modelview.hpp"
#include "log.hpp"

#include <imgui.h>
#include <memory>

ModelView::ModelView(std::shared_ptr<Model> model)
    : m_model(model)
{
    mlog::logln(mlog::INFO, "opening model view.");
}



void ModelView::draw_scene(Viewport& viewport, InputState& input) {
    glClearColor(0.1, 0.1, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
}

void ModelView::draw_ui(InputState& input) {
    ImGui::Begin("Model View");

    if(ImGui::Button("Exit")) {
        mlog::logln(mlog::INFO, "exiting model view");
        m_remove = true;
    }

    ImGui::End();
}

void ModelView::menu_item() {
    if(ImGui::BeginMenu("View")) {
        if(ImGui::MenuItem("Exit")) {
            mlog::logln(mlog::INFO, "exiting model view");
            m_remove = true;
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Export")) {
        if(ImGui::MenuItem("Export as 3D-Object (.obj)")) {
            mlog::logln(mlog::ERROR, "model export unimplemented!");
        }

        ImGui::EndMenu();
    }
}
