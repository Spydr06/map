#pragma once

#include "renderutil.hpp"
#include <memory>

class Overlay : public RenderElement {
public:
    Overlay();
    ~Overlay();

    virtual void draw_scene(Viewport& viewport, InputState& input) override;
    virtual void draw_ui(InputState& input) override;
    
    virtual int get_z_index() const override {
        return 1;
    }

private:
    std::unique_ptr<Shader> m_shader;

    GLuint m_vao = 0, m_vbo = 0;
};

