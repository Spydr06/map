BUILD_DIR ?= ./build
INCLUDE_DIR ?= ./include

IMGUI_DIR ?= ./imgui

NFD_DIR ?= ./nativefiledialog
NFD_INCLUDE_DIR ?= $(NFD_DIR)/src/include
NFD_BUILD_DIR ?= $(NFD_DIR)/build/gmake_linux

NFD_OBJECT_DIR ?= $(NFD_DIR)/build/obj/x64/Release/nfd
NFD_OBJECTS ?= $(NFD_OBJECT_DIR)/nfd_common.o $(NFD_OBJECT_DIR)/nfd_gtk.o

BINARY ?= $(BUILD_DIR)/map

LIBRARIES := expat glfw3 glew glm gtk+-3.0 libcurl librsvg-2.0 cairo libgeotiff

SOURCES := $(wildcard $(IMGUI_DIR)/*.cpp) $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp $(wildcard *.cpp) 
OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES)) $(NFD_OBJECTS)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.hpp)

CXXFLAGS += -Wall -Wextra -pedantic -std=c++23 $(shell pkg-config --cflags $(LIBRARIES)) -Iinclude -I$(IMGUI_DIR) -I$(NFD_INCLUDE_DIR) -ggdb
LDFLAGS += $(shell pkg-config --libs $(LIBRARIES)) -lm -ltiff

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF "$(@:%.o=%.d)" -c $< -o $@

.PHONY: nfd
nfd: $(NFD_DIR)
	$(MAKE) -C $(NFD_BUILD_DIR) nfd

$(NFD_OBJECT_DIR)/%.o: nfd
	true

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(NFD_BUILD_DIR) clean

