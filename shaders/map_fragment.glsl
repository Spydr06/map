#version 450 core

in vec4 v_Color;

layout (location = 0) out vec4 frag_Color;

void main() {
    frag_Color = v_Color;
}

