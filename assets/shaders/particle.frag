#version 460 core

out vec4 FragColor;
in float v_Density;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    if (dist > 0.5) discard;

    //Soft water droplet styling
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = vec4(0.15, 0.55, 0.95, alpha * 0.8);
}