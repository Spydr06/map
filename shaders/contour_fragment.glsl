#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

uniform sampler2D u_Texture;
uniform float u_Epsilon;
uniform float u_Spacing;
uniform vec2 u_HeightRange;
uniform vec4 u_Color;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

void main() {
    /*float d = 0.001;

    vec2 texel = vec2(d);
    vec2 uv = clamp(v_TexCoord, texel, 1.0 - texel);

    float h = texture(u_Texture, uv).r;

    float u = texture(u_Texture, uv + vec2(0.0, d)).r;
    float v = texture(u_Texture, uv + vec2(d, 0.0)).r;

    vec2 grad = vec2(u - h, v - h);
    float dh = inversesqrt(dot(grad, grad));

    float contour = abs(fract((h - u_HeightRange.x) / u_Spacing) - 0.5) * u_Spacing;
    float line = step(contour * dh, u_Epsilon);

    float brightness = map(h, u_HeightRange.x, u_HeightRange.y, 0.25, 0.90);

    frag_Color = vec4(vec3(line * brightness), 1.0);*/

    float h = texture(u_Texture, v_TexCoord).r;

    float level = (h - u_HeightRange.x) / u_Spacing;
    float distToLine = abs(fract(level) - 0.5);

    float width = fwidth(level);
    float aa = width * u_Epsilon;

    float line = 1.0 - smoothstep(0.0, aa, distToLine);

    float brightness = map(h, u_HeightRange.x, u_HeightRange.y, 0.0, 1.0);
    frag_Color = vec4(vec3(line * line * brightness), 1.0) * u_Color;
}

