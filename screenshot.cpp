#include "screenshot.hpp"

#include "log.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "viewport.hpp"
#include "main.hpp"

#include "imgui.h"

#include <ctime>
#include <format>
#include <memory>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

struct ResolutionPreset {
    std::string name;
    glm::ivec2 resolution;
};

static const std::map<std::string, glm::ivec2> resolution_presets = {
    { "FHD", RESOLUTION_FHD },
    { "4K", RESOLUTION_4K },
    { "8K", RESOLUTION_8K },
};

void Screenshot::draw_ui(InputState &input) {
    ImGui::Begin("Screenshot", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if(auto& r = m_range) {
        ImGui::Text("Selection: (%f, %f) to (%f, %f) (%f x %f)", r->m_start.x, r->m_start.y, r->m_end.x, r->m_end.y, r->size().x, r->size().y);
    }

    ImGui::SliderFloat("Scale", &m_scale, 0.5f, 8.0f); 

    ImGui::InputInt2("Resolution", reinterpret_cast<int*>(&m_resolution));

    for(auto& [name, resolution] : resolution_presets) {
        if(ImGui::Button(name.c_str())) {
            m_resolution = resolution;
        }
        ImGui::SameLine();
    }

    ImGui::NewLine();
    ImGui::Separator();

    ImGui::Text("Directory: %s", m_directory.string().c_str());

    if(ImGui::Button("Take Screenshot")) {
        m_pending = true;
    }

    ImGui::SameLine();

    if(ImGui::Button("Cancel")) {
        m_remove = true;
    }

    ImGui::End();
}

void Screenshot::draw_ui(Map& map, InputState& input) {
    draw_ui(input);

    if(m_remove) {
        map.deselect_tool();
        m_remove = false;
    }

    if(m_pending) {
        take_screenshot(*context);
    }
}

void Screenshot::take_screenshot(RenderContext& context) {
    m_pending = false;

    mlog::logln(mlog::DEBUG, "start creating screenshot...\n");

    auto filepath = m_directory / std::format("map-{}.jpg", std::time(NULL));

    Framebuffer fb()

    // target texture
    GLuint target = 0;
    glGenTextures(1, &target);
    assert(target != 0);

    glBindTexture(GL_TEXTURE_2D, target);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_resolution.x, m_resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // render buffer
    GLuint rbo = 0;
    glGenRenderbuffers(1, &rbo);
    assert(rbo != 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_resolution.x, m_resolution.y);

    // frame buffer
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    assert(fbo != 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    GLenum draw_buffers[] = {
        GL_COLOR_ATTACHMENT0
    };
    glDrawBuffers(1, draw_buffers);

    std::unique_ptr<uint8_t[]> data = nullptr;
    glm::vec3 clear_color = context.get_clear_color();

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        mlog::logln(mlog::ERROR, "failed generating framebuffer.");

        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rbo);
        glDeleteTextures(1, &target);
        return;
    }

    glViewport(0, 0, m_resolution.x, m_resolution.y);

    glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);

    Viewport &viewport = context.get_viewport();
    float scale_before = viewport.get_scale_factor();
    viewport.get_scale_factor() = m_scale;

    context.draw_scene();

    viewport.get_scale_factor() = scale_before;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    data = std::make_unique<uint8_t[]>(m_resolution.x * m_resolution.y * sizeof(uint8_t) * 4);
    glBindTexture(GL_TEXTURE_2D, target);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

    stbi_flip_vertically_on_write(true);
    if(!stbi_write_jpg(filepath.c_str(), m_resolution.x, m_resolution.y, 4, data.get(), 100)) {
        mlog::logln(mlog::ERROR, "failed writing image to %s: %s", filepath.c_str(), strerror(errno));
        goto cleanup;
    }

    mlog::logln(mlog::INFO, "saved screenshot to \"%s\".", filepath.c_str());

cleanup:
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &target);
}

