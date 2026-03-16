#version 460 core

layout(location = 0) in vec4 aPos; // xyz = position, w = density

uniform mat4 view;
uniform mat4 projection;
uniform float u_particleRadius; // The physical size of the water drop

out vec3 v_ViewPos;
out float v_Radius;

void main() {
    vec4 viewPos = view * vec4(aPos.xyz, 1.0);
    v_ViewPos = viewPos.xyz;
    v_Radius = u_particleRadius;

    gl_Position = projection * viewPos;

    // Dynamically scale the point size based on projection matrix and camera distance
    gl_PointSize = (projection[1][1] * u_particleRadius * 500.0) / -viewPos.z;
}