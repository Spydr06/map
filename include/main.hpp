#pragma once

#include "rendercontext.hpp"

#include <memory>
#include <GLFW/glfw3.h>

#define VERSION_STRING "0.1.0"

#define SETTINGS_DEFAULT_FILENAME "map-settings.ini"

extern std::unique_ptr<RenderContext> context;

