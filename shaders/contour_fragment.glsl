#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

layout(binding = 0) uniform sampler2D u_Tiles[9];
uniform ivec2 u_TileSize;

uniform float u_Epsilon;
uniform float u_Spacing;
uniform vec2 u_HeightRange;
uniform vec3 u_BackgroundColor;
uniform vec4 u_Color;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

float fetch_tile(ivec2 p)
{
    ivec2 tile = ivec2(floor(vec2(p) / vec2(u_TileSize)));

    ivec2 local = p - tile * u_TileSize;
    int index = (tile.y + 1) * 3 + (tile.x + 1);

    return texelFetch(u_Tiles[index], local, 0).r;
}

float interpolate_height(vec2 uv) {
    vec2 pixel = uv * vec2(u_TileSize);

    ivec2 p = ivec2(floor(pixel));
    vec2 f = fract(pixel);

    float h00 = fetch_tile(p);
    float h10 = fetch_tile(p + ivec2(1, 0));
    float h01 = fetch_tile(p + ivec2(0, 1));
    float h11 = fetch_tile(p + ivec2(1, 1));

    return mix(mix(h00, h10, f.x), mix(h01, h11, f.x), f.y);
}

float contour_line(float spacing, float epsilon, float h) {
    float level = h / spacing;
    float f = fract(level);
    float distToLine = min(f, 1.0 - f);

    float width = length(vec2(dFdx(h), dFdy(h))) / spacing;
    float aa = max(width * epsilon, 1e-6);

    return 1.0 - smoothstep(0.0, aa, distToLine);
}

void main() {
    float h = interpolate_height(v_TexCoord);

    float minorLine = contour_line(u_Spacing, u_Epsilon, h) * 0.8;
    float majorLine = contour_line(u_Spacing * 5.0, u_Epsilon * 1.6, h);

    float line = max(minorLine, majorLine);

    vec3 lineColor = u_Color.xyz;
    frag_Color = vec4(mix(u_BackgroundColor, lineColor, line), 1.0);
}

