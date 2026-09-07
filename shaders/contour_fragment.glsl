#version 450 core

in vec2 v_TexCoord;

layout (location = 0) out vec4 frag_Color;

layout(binding = 0) uniform sampler2D u_Tiles[9];
uniform ivec2 u_TileSize;

uniform float u_Epsilon;
uniform float u_Spacing;
uniform int u_HlFrequency;
uniform float u_HlMultiply;
uniform vec2 u_HeightRange;
uniform vec3 u_BackgroundColor;
uniform vec4 u_Color;
uniform vec2 u_Scale;
uniform float u_ScaleFactor;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

float fetch_tile(ivec2 p) {
    ivec2 tile = ivec2(floor(vec2(p) / vec2(u_TileSize)));

    ivec2 local = p - tile * u_TileSize;
    int index = (tile.y + 1) * 3 + (tile.x + 1);

    float lod = clamp(log2(length(u_Scale)), 0.0, 1.0);
    vec2 uv = (vec2(local) + 0.5) / vec2(u_TileSize);
    // return texelFetch(u_Tiles[index], local, 0).r;
    return textureLod(u_Tiles[index], uv, lod).r;
}

// catmull-rom interpolation
float cubic(float v0, float v1, float v2, float v3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5 * (
        2.0 * v1 +
        (-v0 + v2) * t +
        (2.0 * v0 - 5.0 * v1 + 4.0 * v2 - v3) * t2 +
        (-v0 + 3.0 * v1 - 3.0 * v2 + v3) * t3
    );
}

float cubic_derivative(float v0, float v1, float v2, float v3, float t) {
    float t2 = t * t;

    return 0.5 * (
        (-v0 + v2) +
        2.0 * (2.0 * v0 - 5.0 * v1 + 4.0 * v2 - v3) * t +
        3.0 * (-v0 + 3.0 * v1 - 3.0 * v2 + v3) * t2
    );
}

vec3 interpolate_gradient(vec2 uv) {
    vec2 pixel = uv * vec2(u_TileSize);

    ivec2 p = ivec2(floor(pixel));
    vec2 f = fract(pixel);

    float row0 = cubic(
        fetch_tile(p + ivec2(-1, -1)),
        fetch_tile(p + ivec2( 0, -1)),
        fetch_tile(p + ivec2( 1, -1)),
        fetch_tile(p + ivec2( 2, -1)),
        f.x
    );

    float row1 = cubic(
        fetch_tile(p + ivec2(-1, 0)),
        fetch_tile(p + ivec2( 0, 0)),
        fetch_tile(p + ivec2( 1, 0)),
        fetch_tile(p + ivec2( 2, 0)),
        f.x
    );

    float row2 = cubic(
        fetch_tile(p + ivec2(-1, 1)),
        fetch_tile(p + ivec2( 0, 1)),
        fetch_tile(p + ivec2( 1, 1)),
        fetch_tile(p + ivec2( 2, 1)),
        f.x
    );

    float row3 = cubic(
        fetch_tile(p + ivec2(-1, 2)),
        fetch_tile(p + ivec2( 0, 2)),
        fetch_tile(p + ivec2( 1, 2)),
        fetch_tile(p + ivec2( 2, 2)),
        f.x
    );

    float row0_dx = cubic_derivative(
        fetch_tile(p + ivec2(-1, -1)),
        fetch_tile(p + ivec2( 0, -1)),
        fetch_tile(p + ivec2( 1, -1)),
        fetch_tile(p + ivec2( 2, -1)),
        f.x
    );

    float row1_dx = cubic_derivative(
        fetch_tile(p + ivec2(-1, 0)),
        fetch_tile(p + ivec2( 0, 0)),
        fetch_tile(p + ivec2( 1, 0)),
        fetch_tile(p + ivec2( 2, 0)),
        f.x
    );

    float row2_dx = cubic_derivative(
        fetch_tile(p + ivec2(-1, 1)),
        fetch_tile(p + ivec2( 0, 1)),
        fetch_tile(p + ivec2( 1, 1)),
        fetch_tile(p + ivec2( 2, 1)),
        f.x
    );

    float row3_dx = cubic_derivative(
        fetch_tile(p + ivec2(-1, 2)),
        fetch_tile(p + ivec2( 0, 2)),
        fetch_tile(p + ivec2( 1, 2)),
        fetch_tile(p + ivec2( 2, 2)),
        f.x
    );

    float h = cubic(row0, row1, row2, row3, f.y);

    // ∂h/∂pixel.x
    float dhdx = cubic(row0_dx, row1_dx, row2_dx, row3_dx, f.y);

    // ∂h/∂pixel.y
    float dhdy = cubic_derivative(row0, row1, row2, row3, f.y);

    // Convert from derivatives per pixel to derivatives per UV.
    dhdx *= float(u_TileSize.x);
    dhdy *= float(u_TileSize.y);

    return vec3(h, dhdx, dhdy);
}

float height_span() {
    return abs(u_HeightRange.x - u_HeightRange.y);
}

float contour_line(float spacing, float epsilon, vec3 grad) {
    /*float h = grad.x;
    vec2 dh = grad.yz / height_span();

    float level = h / spacing;
    float f = fract(level);
    float distToLine = min(f, 1.0 - f);

    float width = length(dh) / spacing;
    float aa = max(width * epsilon, 1e-6);

    return 1.0 - smoothstep(0.0, aa, distToLine);*/
    double h = grad.x;
    dvec2 dh = dvec2(grad.yz) / height_span();

    double level = h / spacing;
    double f = fract(level);
    double distToLine = min(f, 1.0 - f);

    // local height gradient magnitude
    double g = length(dh);
    double zoom = length(u_Scale) * 0.1;

    // convert desired visual width to contour-space width
    double halfWidth = 0.5 * epsilon * g / (spacing * zoom);

    double aa = max(halfWidth, 1e-12);

    double hard = step(distToLine, halfWidth);
    double edge = 1.0 - smoothstep(halfWidth, halfWidth + aa, distToLine);

    return float(max(hard, edge));
}

void main() {
    vec3 grad = interpolate_gradient(v_TexCoord);

    float minorLine = contour_line(u_Spacing, u_Epsilon, grad);
    float majorLine = float(u_HlFrequency > 0)
        * contour_line(u_Spacing * abs(float(u_HlFrequency)), u_Epsilon * u_HlMultiply, grad);

    float line = max(minorLine, majorLine);

    vec3 lineColor = u_Color.xyz;
    frag_Color = vec4(mix(u_BackgroundColor, lineColor, line), 1.0);
}


