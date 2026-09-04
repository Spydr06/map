#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

layout(binding = 0) uniform sampler2D u_Tiles[9];
uniform ivec2 u_TileSize;

uniform vec2 u_HeightRange;

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

void main() {
    float h = map(interpolate_height(v_TexCoord), u_HeightRange.x, u_HeightRange.y, 0.0, 1.0);

    frag_Color = vec4(vec3(clamp(h*h, 0.0, 1.0)), 1.0);
}

