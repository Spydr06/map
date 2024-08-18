#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in uint a_Metadata;

uniform vec2 u_Scale;
uniform vec2 u_Translation;

flat out vec2 v_StartPos;
out vec2 v_VertPos;

void main() {
    v_VertPos = (a_Position + u_Translation) * u_Scale;
    v_StartPos = v_VertPos;
    gl_Position = vec4(
        v_VertPos, 
        1.0,
        1.0
    );
}

