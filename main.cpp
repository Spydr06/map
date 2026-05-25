#include <chrono>
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <memory>

#include "heightmap.hpp"
#include "log.hpp"
#include "map.hpp"
#include "overlay.hpp"
#include "preprocess.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "screenshot.hpp"
#include "taginfo.hpp"
#include "timer.hpp"

#include <GLFW/glfw3.h>
#include <getopt.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

constexpr glm::vec2 window_size = glm::vec2(1366, 768);

std::unique_ptr<RenderContext> context = nullptr;

auto main(int argc, char** argv) -> int {
    mlog::init_from_env("MAP_LOG");

    const char *taginfo_path = nullptr;
    const char *heightmap_path = nullptr;

    int opt;
    while((opt = getopt(argc, argv, "t:h:")) != EOF) {
        switch(opt) {
        case 'h':
            heightmap_path = optarg;
            break;
        case 't':
            taginfo_path = optarg;
            break;
        default:
            mlog::logln(mlog::ERROR, "Usage: %s <osm xml file> [-t <taginfo xml file>] [-h <heightmap geotiff>]", argv[0]);
            return 1;
        }
    }

    if(optind != argc - 1) {
        mlog::logln(mlog::ERROR, "Usage: %s <osm xml file> [-t <taginfo xml file>] [-h <heightmap geotiff>]", argv[0]);
        return 1;
    }

    const char *osm_path = argv[optind];

    if(!glfwInit()) {
        mlog::logln(mlog::ERROR, "Error initializing GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "Map", nullptr, nullptr);
    if(!window) {
        mlog::logln(mlog::ERROR, "Error creating GLFW window");
        glfwTerminate();
        return 1;
    }

    auto timers = std::vector({
        Timer(std::chrono::seconds(1), [](auto& frame_time){
            mlog::logln(mlog::DEBUG, "fps: %ld", std::chrono::seconds(1) / frame_time);
        })
    });

    auto last_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration frame_time;

    glfwMakeContextCurrent(window);
    
    //glfwSwapInterval(0);

    if(GLenum err = glewInit()) {
        mlog::logln(mlog::ERROR, "OpenGL error: %s", glewGetErrorString(err));
        glfwTerminate();
        return 1;
    }

    auto map = std::make_shared<Map>();

    mlog::logln(mlog::INFO, "Preprocessing data...");

    if(int err; taginfo_path && (err = load_taginfo(taginfo_path, map))) {
        return err;
    }

    if(heightmap_path) {
        auto heightmap = std::make_shared<Heightmap>(std::string(heightmap_path));
        if(int err = heightmap->preprocess()) {
            return err;
        }

        map->set_heightmap(heightmap);
    }

    if(int err = preprocess_data(osm_path, map)) {
        return err;
    };

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
    
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    context = std::make_unique<RenderContext>(map, window_size);
    context->add_element(std::make_shared<Overlay>());

    auto screenshot = std::make_shared<Screenshot>();
    context->add_element(screenshot);

    glfwSetWindowContentScaleCallback(window, [](GLFWwindow*, float xscale, float yscale) {
        auto& io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2(xscale, yscale);
    });

    glfwSetScrollCallback(window, [](GLFWwindow*, double xoffset, double yoffset){
        auto& io = ImGui::GetIO();

        if(io.WantCaptureMouse) {
            io.AddMouseWheelEvent(xoffset, yoffset);
            return;
        }

        auto& scale = context->get_viewport().get_scale_factor();
        scale += scale * yoffset * 0.1;
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, [[maybe_unused]] int mods) {
        auto& io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);

        if(io.WantCaptureMouse)
            return;

        switch(button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                context->get_input_state().lmb_down = action == GLFW_PRESS;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                context->get_input_state().rmb_down = action == GLFW_PRESS;
                break;
            default:
                break;
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos) {
        auto& io = ImGui::GetIO();
        io.AddMousePosEvent(xpos, ypos);

        if(io.WantCaptureMouse)
            return;

        glm::vec2 pos(xpos, ypos);

        if(context->get_input_state().lmb_down) {
            context->get_viewport().move((pos - context->get_input_state().last_cursor_pos) * glm::vec2(1.0, -1.0), context->get_input_state().window_size);
        }

        context->get_input_state().set_cursor_pos(pos, context->get_viewport());
    });

    glfwSetWindowSizeCallback(window, [](GLFWwindow*, int width, int height) {
        context->get_input_state().window_size = glm::vec2(width, height);
    });

    while(!glfwWindowShouldClose(window)) {
        for(auto& timer : timers) {
            timer.update(frame_time);
        }
        
        if(glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        glm::vec2 scale;
        glfwGetWindowContentScale(window, &scale.x, &scale.y);

        glm::vec2 window_size = context->get_input_state().window_size * scale;

        glViewport(0, 0, window_size.x, window_size.y);
        
        glm::vec3 clear_color = context->get_clear_color();
        glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_LINE_SMOOTH);

        context->draw_scene();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        
        context->draw_ui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            auto* backup_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_context);
        }

        if(screenshot->pending())
            screenshot->take_screenshot(*context);
        
        glfwSwapBuffers(window);
        glfwWaitEvents();

        auto now = std::chrono::steady_clock::now();
        frame_time = now - last_time;
        last_time = now;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

