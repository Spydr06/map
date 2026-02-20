#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

uniform sampler2D u_Texture;
uniform vec2 u_HeightRange;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

void main() {
    //float c = float(abs(h - u_HeightLine) < 1);

    float h = clamp(
        map(texture(u_Texture, v_TexCoord).r, u_HeightRange.x, u_HeightRange.y, 0.0, 1.0),
        0.0,
        1.0
    );

    frag_Color = vec4(vec3(h * h * h), 1.0);

    /*int c = 0;
    for(int i = int(u_HeightRange.x); i < int(u_HeightRange.y); i += 10) {
        c |= int(abs(h - i) < u_ContourMargin); 
    }

    frag_Color = vec4(vec3(float(c)) * vec3(0.812, 0.494, 0.298), 1.0);*/
}

