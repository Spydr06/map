#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in uint a_Metadata;

uniform vec2 u_Scale;
uniform vec2 u_Translation;
uniform vec4 u_Color;

out vec4 v_Color;

void main() {
    v_Color = u_Color;

    gl_Position = vec4(
        ((a_Position + u_Translation) * u_Scale), 
        1.0,
        1.0
    );
}

