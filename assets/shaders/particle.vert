#version 460 core

layout(location = 0) in vec4 aPos; // xyz = position, w = density

uniform mat4 view;
uniform mat4 projection;

out float v_Density;

void main() {
    v_Density = aPos.w; //TODO: use this later to color the fluid based on pressure

    vec4 viewPos = view * vec4(aPos.xyz, 1.0);
    gl_Position = projection * viewPos;

    //Dynamically scale point size based on distance from the camera
    gl_PointSize = 45.0 / -viewPos.z;
}