#version 460 core

//No VAO attributes - position comes entirely from uniform
//The VAO is a dummy with no data; we draw GL_POINTS with count = 1

uniform mat4 view;
uniform mat4 projection;
uniform vec3 u_position;
uniform float u_pointSize;

void main(){
    gl_Position = projection * view * vec4(u_position, 1.0);
    gl_PointSize = u_pointSize;
}
