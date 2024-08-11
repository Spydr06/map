#pragma once

#include <glm/vec2.hpp>

struct InputState {
    InputState(glm::vec2(window_size))
        : window_size(window_size)
    {}

    bool lmb_down = false;
    bool rmb_down = false;

    glm::vec2 last_cursor_pos = glm::vec2(0);
    glm::vec2 window_size;
};

