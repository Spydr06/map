#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <memory>

#include "preprocess.hpp"
#include "map.hpp"
#include "rendercontext.hpp"
#include "timer.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

auto main(int argc, char** argv) -> int {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <osm xml file>" << std::endl;
        return 1;
    }

    auto map = std::make_shared<Map>();

    auto data_path = "data.txt";
    auto data_file = std::ifstream(data_path);
    if(data_file.good()) {
        std::cout << "Loading data..." << std::endl;
        load_preprocessed_data(data_file, map);
        std::cout << "done." << std::endl;
    }
    else {
        std::cout << "Preprocessing data..." << std::endl;
        if(int err = preprocess_data(argv[1], data_path, map)) {
            return err;
        };
    }

    if(!glfwInit()) {
        std::cerr << "Error initializing GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    GLFWwindow* window = glfwCreateWindow(1366, 768, "Map", nullptr, nullptr);
    if(!window) {
        std::cerr << "Error creating GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    auto timers = std::vector({
        Timer(std::chrono::seconds(1), [](auto& frame_time){
            std::cout << "fps: " << std::chrono::seconds(1) / frame_time << std::endl;
        })
    });

    auto last_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration frame_time;

    glfwMakeContextCurrent(window);

    if(GLenum err = glewInit()) {
        std::cout << "OpenGL error: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
    
    auto context = RenderContext(map);

    while(!glfwWindowShouldClose(window)) {
        for(auto& timer : timers) {
            timer.update(frame_time);
        }
        
        if(glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        context.draw();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

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

