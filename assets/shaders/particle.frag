#version 460 core

out vec4 FragColor;
in float v_Density;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5) discard;
    
    // Map density to color (Target density is roughly 1000)
    // High pressure = white/cyan. Low pressure = deep blue
    float pressureRatio = clamp((v_Density - 800.0) / 600.0, 0.0, 1.0);
    vec3 color = mix(vec3(0.0, 0.2, 0.8), vec3(0.6, 0.9, 1.0), pressureRatio);

    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = vec4(color, alpha * 0.9);
}