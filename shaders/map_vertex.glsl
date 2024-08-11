#version 450 core

layout (location = 0) in vec2 a_Position;

uniform vec2 u_Scale;
uniform vec2 u_Translation;

void main() {
    gl_Position = vec4(
        ((a_Position + u_Translation) * u_Scale), 
        1.0,
        1.0
    );
}

