#pragma once

#include "viewport.hpp"
#include <glm/vec2.hpp>

struct InputState {
    InputState(glm::vec2(window_size))
        : window_size(window_size)
    {}

    inline void set_cursor_pos(glm::vec2 cursor_pos, Viewport& viewport) {
        last_cursor_pos = cursor_pos;

        glm::vec2 normalized_pos = cursor_pos / window_size * glm::vec2(2.0) - glm::vec2(1.0);
        glm::vec2 scale = viewport.get_scale(window_size);

        mapped_cursor_pos = -viewport.get_translation() + normalized_pos / scale;
    }

    bool lmb_down = false;
    bool rmb_down = false;

    glm::vec2 last_cursor_pos = glm::vec2(0);
    glm::vec2 mapped_cursor_pos = glm::vec2(0);

    glm::vec2 window_size;
};

