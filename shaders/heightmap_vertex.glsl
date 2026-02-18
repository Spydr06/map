#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoord;

uniform vec2 u_Scale;
uniform vec2 u_Translation;

out vec2 v_TexCoord;

void main() {
    v_TexCoord = a_TexCoord;

    gl_Position = vec4(
        ((a_Position + u_Translation) * u_Scale),
        1.0,
        1.0
    );
}

