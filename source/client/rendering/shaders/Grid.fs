R"(
#version 120
varying vec3 vertexPosition;

uniform vec2 dimensions;
uniform vec2 view_position;
uniform float view_zoom;

#define MAJOR_INTERVAL 16.0
#define INTERVAL_DIVISIONS 4.0

void main()
{
    // Inspired by: https://www.shadertoy.com/view/3ss3RN

    // Translate [-1, 1] position into the position on Soldank map
    vec2 position_on_map = vertexPosition.xy;
    position_on_map *= dimensions.xy;
    position_on_map /= 2.0;

    // Move to the camera's position
    position_on_map += view_position;
    
    // Scale parameters so the line width is always the same,
    // regardless of current zoom
    float minor_line_width = 3.0 * view_zoom;
    float major_line_width = 4.5 * view_zoom;
    float offset = 0.5 * view_zoom;
    
    vec2 minor_vec = abs(mod(position_on_map + minor_line_width / 2.0 + offset, MAJOR_INTERVAL / INTERVAL_DIVISIONS) - offset);
    vec2 major_vec = abs(mod(position_on_map + major_line_width / 2.0 + offset, MAJOR_INTERVAL) - offset);
    
    float minor_intensity = min(minor_vec.x, minor_vec.y);
    float major_intensity = min(major_vec.x, major_vec.y);

    // Make lines disappear if the camera is too far
    if (view_zoom >= 1.0) {
        minor_intensity = 10000.0;
    }

    if (view_zoom >= 4.0) {
        major_intensity = 10000.0;
    }

    // Some AA
    float color_intensity = smoothstep(0., 1.5 * view_zoom, min(minor_intensity, major_intensity));
    
    gl_FragColor = vec4(vec3(color_intensity), 1.0 - color_intensity);
}
)"