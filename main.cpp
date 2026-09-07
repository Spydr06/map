#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <memory>

#include "main.hpp"
#include "heightmap.hpp"
#include "log.hpp"
#include "map.hpp"
#include "overlay.hpp"
#include "preprocess.hpp"
#include "query.hpp"
#include "rendercontext.hpp"
#include "renderutil.hpp"
#include "screenshot.hpp"
#include "settings.hpp"
#include "taginfo.hpp"
#include "timer.hpp"

#include <GLFW/glfw3.h>
#include <getopt.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

std::unique_ptr<RenderContext> context = nullptr;

[[noreturn]]
static void usage(const char *progname) {
    mlog::logln(mlog::ERROR, "Usage: %s [<osm xml file>] [-t <taginfo xml file>] [-h <heightmap geotiff>] [-s <settings ini>]", progname);
    std::exit(EXIT_SUCCESS);
}

static void set_style(ImGuiStyle& s);

class Help : public RenderElement {
public:
    virtual void menu_item() override;

    virtual void draw_scene(Viewport&, InputState&) override {};
    virtual void draw_ui(InputState& input) override;

    virtual int get_z_index() const override {
        return 100;
    }
private:
    void about_dialog();

    bool m_about_showing = false;
};

void Help::menu_item() {
    if(ImGui::BeginMenu("Help")) {
        if(ImGui::MenuItem("About Mapviewer")) {
            m_about_showing = true;
        }

        ImGui::EndMenu();
    }
}

void Help::draw_ui(InputState& input) {
    if(m_about_showing)
        about_dialog();
}

void Help::about_dialog() {
    static char about_text[] = 
R"(OpenStreetMap (OSM) Viewer and Heightmap generator.

Copyright (C) 2026 Spydr06
Licensed under the MIT License.

This is free software; see the source for copying conditions.
There is NO warranty.

Source code: https://github.com/spydr06/map
No AI used in the creation of this software.

Version: )" VERSION_STRING R"(

Thirdparty Libraries:

- cairo: https://cairographics.org/download
- glew: https://glew.sourceforge.ne
- glfw: https://www.glfw.org
- glm: https://github.com/g-truc/glm
- imgui: https://github.com/ocornut/imgui
- inipp: https://github.com/mcmtroffaes/inipp
- libcurl: https://curl.se/libcurl
- libexpat: https://libexpat.github.io
- libgeotiff: https://github.com/OSGeo/libgeotiff
- librsvg: https://gitlab.gnome.org/GNOME/librsvg
- nativefiledialog: https://github.com/mlabbe/nativefiledialog
and depending libraries.

These projects are not affiliated with me in any way.
)";

    ImGui::Begin("About", &m_about_showing);

    ImGui::InputTextMultiline("##about_text", about_text, IM_COUNTOF(about_text), ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

    ImGui::End();
}

auto main(int argc, char** argv) -> int {
    mlog::init_from_env("MAP_LOG");

    const char *osm_path = nullptr;
    const char *taginfo_path = nullptr;
    const char *heightmap_path = nullptr;

    auto home = std::getenv("HOME");
    assert(home && "$HOME Environment variable not set.");
    
    std::filesystem::path settings_path = home;
    settings_path /= ".config";
    settings_path /= SETTINGS_DEFAULT_FILENAME;

    int opt;
    while((opt = getopt(argc, argv, "t:h:s:")) != EOF) {
        switch(opt) {
        case 'h':
            heightmap_path = optarg;
            break;
        case 't':
            taginfo_path = optarg;
            break;
        case 's':
            settings_path = std::filesystem::path(optarg);
            break;
        default:
            usage(argv[0]);
        }
    }

    if(optind < argc - 1) {
        usage(argv[1]);
    }
    else if(optind == argc - 1) {
        osm_path = argv[optind];
    }

    settings_store = std::make_shared<ini_store>(settings_path);

    if(!osm_path && taginfo_path) {
        mlog::logln(mlog::ERROR, "Cannot load tag information without an OSM map");
        usage(argv[1]);
    }

    if(!glfwInit()) {
        mlog::logln(mlog::ERROR, "Error initializing GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    static const auto window_width = std::make_unique<non_volatile<int, "window.width">>(1366);
    static const auto window_height = std::make_unique<non_volatile<int, "window.height">>(768);
    
    GLFWwindow* window = glfwCreateWindow(*window_width, *window_height, "Map", nullptr, nullptr);
    if(!window) {
        mlog::logln(mlog::ERROR, "Error creating GLFW window");
        glfwTerminate();
        return 1;
    }

    auto timers = std::vector<Timer>{
        /* Timer(std::chrono::seconds(1), [](auto& frame_time){
            mlog::logln(mlog::DEBUG, "fps: %ld", std::chrono::seconds(1) / frame_time);
        }) */
    };

    auto last_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration frame_time;

    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
    if(GLenum err = glewInit()) {
        mlog::logln(mlog::ERROR, "OpenGL error: %s", glewGetErrorString(err));
        glfwTerminate();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable 
                   | ImGuiConfigFlags_ViewportsEnable
                   | ImGuiConfigFlags_DpiEnableScaleFonts 
                   | ImGuiConfigFlags_DpiEnableScaleViewports;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;
    
    auto& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    set_style(style);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");

    context = std::make_unique<RenderContext>(window, glm::vec2(**window_width, **window_height));

    if(heightmap_path) {
        auto heightmap = std::make_shared<Heightmap>(std::string(heightmap_path));
        if(int err = heightmap->preprocess()) {
            return err;
        }

        context->add_element(heightmap);
    }

    std::shared_ptr<Map> map = nullptr;
    if(osm_path) {
        mlog::logln(mlog::INFO, "Preprocessing data...");

        map = std::make_shared<Map>(); 

        if(int err = preprocess_data(osm_path, map))
            return err;
        
        map->rebuild_vaos();
        context->add_element(map);

        if(int err; taginfo_path && (err = load_taginfo(taginfo_path, map))) {
            return err;
        }

    }

    context->add_element(std::make_shared<MapLoader>());
    context->add_element(std::make_shared<MapView>());
    context->add_element(std::make_shared<Overlay>());
    context->add_element(std::make_shared<Console>());
    context->add_element(std::make_shared<MapQuery>());
    context->add_element(std::make_shared<Help>());

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

        auto& scale = context->get_viewport().get_zoom_factor();
        scale += scale * yoffset * 0.1;
        scale = std::max(0.01f, scale);
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
        *window_width = width;
        *window_height = height;
        context->get_input_state().window_size = glm::vec2(width, height);
    });

    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);

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

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        context->draw_ui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* context_save = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(context_save);
        }

        if(auto screenshot = context->get_element<Screenshot>()) {
            if(screenshot->pending())
                screenshot->take_screenshot(*context);
        }

        context->remove_elements();

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

static void set_style(ImGuiStyle& style) {
    style.Alpha = 0.9f;
	style.DisabledAlpha = 0.8f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 4.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 4.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 4.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 4.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 13.0f;
	style.ScrollbarRounding = 12.0f;
	style.GrabMinSize = 7.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 1.0f;
	// style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1764706f, 0.1764706f, 0.1764706f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2784314f, 0.2784314f, 0.2784314f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.30980393f, 0.30980393f, 0.30980393f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.2627451f, 0.2627451f, 0.2627451f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2784314f, 0.2784314f, 0.2784314f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14509805f, 0.14509805f, 0.14509805f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14509805f, 0.14509805f, 0.14509805f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14509805f, 0.14509805f, 0.14509805f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.19215687f, 0.19215687f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.27450982f, 0.27450982f, 0.27450982f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.29803923f, 0.29803923f, 0.29803923f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3882353f, 0.3882353f, 0.3882353f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.156f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.391f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.30980393f, 0.30980393f, 0.30980393f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.46666667f, 0.46666667f, 0.46666667f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.46666667f, 0.46666667f, 0.46666667f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.2627451f, 0.2627451f, 0.2627451f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3882353f, 0.3882353f, 0.3882353f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.55f, 0.4f, 0.8f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.09411765f, 0.09411765f, 0.09411765f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.34901962f, 0.34901962f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.19215687f, 0.19215687f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.09411765f, 0.09411765f, 0.09411765f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19215687f, 0.19215687f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.46666667f, 0.46666667f, 0.46666667f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.58431375f, 0.58431375f, 0.58431375f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.156f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.55f, 0.4f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.586f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.586f);
}


