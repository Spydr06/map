BUILD_DIR ?= ./build
IMGUI_DIR ?= ./imgui
INCLUDE_DIR ?= ./include

BINARY ?= $(BUILD_DIR)/map

LIBRARIES := expat glfw3 glew glm libcurl librsvg-2.0 cairo libgeotiff

SOURCES := $(wildcard $(IMGUI_DIR)/*.cpp) $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp $(wildcard *.cpp) 
OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
HEADERS := $(wildcard $(INCLUDE_DIR)/*.hpp)

CXXFLAGS += -Wall -Wextra -pedantic -std=c++23 $(shell pkg-config --cflags $(LIBRARIES)) -Iinclude -I$(IMGUI_DIR)
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES)) -lm -ltiff

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp | $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF "$(@:%.o=%.d)" -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
