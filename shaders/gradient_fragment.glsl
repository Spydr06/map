#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

layout(binding = 0) uniform sampler2D u_Tiles[9];
uniform ivec2 u_TileSize;

uniform float u_Delta;
uniform float u_Brightness;

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
    float d = u_Delta * 0.01;

    float h = interpolate_height(v_TexCoord);

    float u = interpolate_height(v_TexCoord + vec2(0.0, d));
    float v = interpolate_height(v_TexCoord + vec2(d, 0.0));

    float dh = sqrt((h-u) * (h-u) + (h-v) * (h-v));
    frag_Color = vec4(vec3(dh * u_Brightness * 0.1), 1.0);
}

