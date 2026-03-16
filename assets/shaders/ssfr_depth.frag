#version 460 core

in vec3 v_ViewPos;
in float v_Radius;

out vec4 FragColor;

uniform mat4 projection;

void main() {
    // gl_PointCoord goes from [0, 1]. We remap it to [-1, 1] to get the center of the sphere.
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    
    // Discard the corners of the square so we get a perfect circle
    if (r2 > 1.0) discard;

    // Ray-Sphere Intersection: Calculate the exact Z-thickness of the sphere at this pixel
    float z = sqrt(1.0 - r2);
    
    // Push the View-Space position forward along the Z axis to create the 3D curve
    vec3 sphereViewPos = v_ViewPos;
    sphereViewPos.z += z * v_Radius; 

    // Project this curved 3D surface into Clip Space
    vec4 clipPos = projection * vec4(sphereViewPos, 1.0);
    float depth = (clipPos.z / clipPos.w);
    
    // Write the perfect 3D depth to the Framebuffer!
    gl_FragDepth = depth * 0.5 + 0.5;
    
    // Also write the linear view-space depth to the color texture. 
    // We will use this in the next pass to blur the water together!
    FragColor = vec4(-sphereViewPos.z, 0.0, 0.0, 1.0);
}