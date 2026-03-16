#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_colorTexture; // Holds linear view-space Z
uniform mat4 u_projection; // Holds gl_FragDepth
uniform mat4 u_invProjection;     // Used to reconstruct the 3D surface

// Reconstruct the exact 3D View-Space position of the water from the 2D screen coordinate
vec3 reconstructViewPos(vec2 uv, float linearDepth) {
    // Project a ray from the camera through the pixel
    vec4 clipPos = vec4(uv * 2.0 - 1.0, -1.0, 1.0); 
    vec4 viewPos = u_invProjection * clipPos;
    viewPos.xyz /= viewPos.w;
    
    // Scale the ray exactly to our fluid depth
    float actualZ = -linearDepth; // We stored absolute depth, but OpenGL view space looks down -Z 
    return viewPos.xyz * (actualZ / viewPos.z);
}

void main() {
    float viewZ = texture(u_colorTexture, TexCoords).r;
    if (viewZ == 0.0) discard;

    vec2 texelSize = 1.0 / textureSize(u_colorTexture, 0);
    
    // THE SECRET SAUCE: Sample further away to low-pass filter the bumps!
    vec2 offset = texelSize * 3.0; 
    
    // Sample the depth 3 pixels to our right and top
    float zRight = texture(u_colorTexture, TexCoords + vec2(offset.x, 0.0)).r;
    float zUp    = texture(u_colorTexture, TexCoords + vec2(0.0, offset.y)).r;
    
    if (zRight == 0.0) zRight = viewZ;
    if (zUp == 0.0) zUp = viewZ;

    // Build the 3D triangle using the wider offsets
    vec3 posCurrent = reconstructViewPos(TexCoords, viewZ);
    vec3 posRight   = reconstructViewPos(TexCoords + vec2(offset.x, 0.0), zRight);
    vec3 posUp      = reconstructViewPos(TexCoords + vec2(0.0, offset.y), zUp);

    vec3 ddx = posRight - posCurrent;
    vec3 ddy = posUp - posCurrent;
    vec3 normal = normalize(cross(ddx, ddy));

    // --- BLINN-PHONG LIGHTING ---
    vec3 lightDir = normalize(vec3(0.5, 0.8, 1.0)); // Sunlight coming from above/right
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular Highlight (The shiny sun reflection)
    vec3 viewDir = normalize(-posCurrent);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 128.0);

    // Deep Ocean Scattering
    vec3 waterColor = vec3(0.02, 0.25, 0.65); 
    vec3 ambient = waterColor * 0.4;
    vec3 foam = vec3(spec * 1.5); 
    
    vec3 finalColor = ambient + (waterColor * diff) + foam;

    // Write the depth so mountains occlude the water properly
    vec4 clipPos = u_projection * vec4(0.0, 0.0, -viewZ, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;
    
    // Output the final liquid color with 85% opacity
    FragColor = vec4(finalColor, 0.85); 
}