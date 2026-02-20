#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

uniform sampler2D u_Texture;
uniform float u_Delta;
uniform float u_Brightness;

void main() {
    float d = u_Delta * 0.01;

    float h = texture(u_Texture, v_TexCoord).r;

    float u = texture(u_Texture, v_TexCoord + vec2(0.0, d)).r;
    float v = texture(u_Texture, v_TexCoord + vec2(d, 0.0)).r;

    float dh = sqrt((h-u) * (h-u) + (h-v) * (h-v));
    frag_Color = vec4(vec3(dh * u_Brightness * 0.1), 1.0);
}

