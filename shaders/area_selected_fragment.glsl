#version 450 core

layout (location = 0) out vec4 frag_Color;

uniform vec4 u_SelColor;

void main() {
    frag_Color = u_SelColor;
}

