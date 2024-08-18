#version 450 core

layout (location = 0) out vec4 frag_Color;

flat in vec2 v_StartPos;
in vec2 v_VertPos;

uniform vec2 u_Resolution;

const float c_DashSize = 20;
const vec4 c_SelColor = vec4(0.0, 1.0, 1.0, 1.0);

void main() {
    vec2 dir = (v_VertPos - v_StartPos) * u_Resolution / 2.0;
    float dist = length(dir);

    if(fract(dist / c_DashSize * 2.0) > c_DashSize / (c_DashSize * 2.0))
        discard;

    frag_Color = c_SelColor;
}

