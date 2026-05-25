#version 450 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in uint a_Metadata;

uniform vec2 u_Scale;
uniform vec2 u_Translation;

out vec4 v_Color;

const vec4 s_dark = vec4(0.2,0.118,0.22, 1.0);
const vec4 s_mid = vec4(0.439,0.412,0.576, 1.0);
const vec4 s_water = vec4(0.439,0.627,0.686, 1.0);
const vec4 s_light = vec4(0.627,0.757,0.725, 1.0);

const vec4 s_trans = vec4(0.0, 0.0, 0.0, 0.0);

const vec4 c_Colormap[] = vec4[](
    s_light, // unknown
    s_dark, // highway motorway
    s_dark, // highway trunk
    s_dark, // highway primary
    s_dark, // highway secondary
    s_mid, // highway tertiary
    s_mid, // highway unclassified
    s_mid, // highway residential
    s_mid, // living street
    s_mid, // service
    s_mid, // pedestrian
    s_mid, // track
    s_mid, // busway
    s_mid, // footway
    s_mid, // cycleway
    s_mid, // footway sidewalk
    s_mid, // footway crossing

    s_dark, // railway
    s_water, // waterway
    s_water, // lake

    s_trans, // landuse agricultural
    s_light, // landuse forest
    s_light, // landuse industrial
    s_light, // landuse recreational
    s_light, // landuse transport
    s_light, // landuse commercial
    s_light, // landuse residential
    
    s_dark, // aerialway

    s_trans, // power lines
    s_trans, // power distribution

    vec4(1.0, 0.0, 1.0, 1.0)
);

/*const vec4 c_Colormap[] = vec4[](
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
    vec4(0.36, 0.49, 0.89, 1.0), // lake

    vec4(0.58, 0.75, 0.41, 1.0), // landuse agricultural
    vec4(0.24, 0.36, 0.22, 1.0), // landuse forest
    vec4(0.89, 0.55, 0.62, 1.0), // landuse industrial
    vec4(0.58, 0.75, 0.41, 1.0), // landuse recreational
    vec4(0.89, 0.55, 0.62, 1.0), // landuse transport
    vec4(0.89, 0.55, 0.62, 1.0), // landuse commercial
    vec4(0.3, 0.3, 0.3, 0.5), // landuse residential
    
    vec4(0.85, 0.28, 0.28, 1.0), // aerialway

    vec4(0.46, 0.18, 0.63, 1.0), // power lines
    vec4(0.46, 0.18, 0.63, 1.0), // power distribution

    vec4(1.0, 0.0, 1.0, 1.0)
);*/


void main() {
    v_Color = c_Colormap[a_Metadata & 0xff];

    gl_Position = vec4(
        ((a_Position + u_Translation) * u_Scale), 
        1.0,
        1.0
    );
}

