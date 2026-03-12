#version 460 core

out vec4 FragColor;

uniform vec3 u_color;
uniform bool u_selected;

void main() {
    // gl_PointCoord is in [0,1] over the point sprite quad.
    // Remap to [-0.5, 0.5] and measure distance from center.
    vec2  coord = gl_PointCoord - vec2(0.5);
    float dist  = length(coord);

    // Discard fragments outside the circle
    if (dist > 0.5) discard;

    // Selection ring: white band between radius 0.36 and 0.48
    if (u_selected && dist > 0.36 && dist < 0.48) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    // Inner filled circle with soft glow falloff
    float alpha = 1.0 - smoothstep(0.0, 0.42, dist);
    FragColor   = vec4(u_color, alpha);
}