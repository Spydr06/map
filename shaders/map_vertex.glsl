#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in uint a_Metadata;

uniform vec2 u_Scale;
uniform vec2 u_Translation;

out vec4 v_Color;

const vec4 c_Colormap[] = vec4[](
    vec4(0.3, 0.3, 0.3, 0.5), // unknown
    vec4(1.00, 0.32, 0.31, 1.0), // highway motorway
    vec4(1.00, 0.56, 0.31, 1.0), // highway trunk
    vec4(1.00, 0.71, 0.31, 1.0), // highway primary
    vec4(1.00, 0.87, 0.52, 1.0), // highway secondary
    vec4(0.77, 0.77, 0.77, 1.0), // highway tertiary
    vec4(0.70, 0.70, 0.70, 1.0), // highway unclassified
    vec4(0.77, 0.77, 0.77, 1.0), // highway residential
    vec4(0.55, 0.75, 0.89, 1.0), // living street
    vec4(0.33, 0.33, 0.33, 1.0), // service
    vec4(0.33, 0.69, 0.55, 1.0), // pedestrian
    vec4(0.48, 0.40, 0.30, 1.0), // track
    vec4(0.32, 0.34, 0.55, 1.0), // busway
    vec4(0.50, 0.50, 0.50, 1.0), // footway
    vec4(0.50, 0.40, 0.59, 1.0), // cycleway
    vec4(0.50, 0.50, 0.50, 1.0), // footway sidewalk
    vec4(1.0), // footway crossing

    vec4(1.0), // railway
    vec4(0.36, 0.49, 0.89, 1.0), // waterway
    vec4(1.0, 0.0, 1.0, 1.0)
);

void main() {
    v_Color = c_Colormap[a_Metadata & 0xff];

    gl_Position = vec4(
        ((a_Position + u_Translation) * u_Scale), 
        1.0,
        1.0
    );
}

