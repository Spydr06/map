#pragma once

#include "renderutil.hpp"

class ModelView : public RenderElement {
public:
    ModelView(std::shared_ptr<Model> model);

    virtual void menu_item() override;
    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;

    virtual bool remove() const override {
        return m_remove;
    }

    int get_z_index() const override {
        return 1;
    }

    bool translucent() const override {
        // overwrites the 2D map
        return false;
    }

private:
    std::shared_ptr<Model> m_model;

    bool m_remove = false;
};

