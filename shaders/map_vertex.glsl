#version 450 core

layout (location = 0) in vec2 a_Position;

uniform vec2 u_MinViewport;
uniform vec2 u_MaxViewport;

void main() {
    vec2 scale = vec2(2.0) / (u_MaxViewport - u_MinViewport);
    gl_Position = vec4(
        ((a_Position - u_MinViewport) * scale) - vec2(1), 
        1.0,
        1.0
    );
}

