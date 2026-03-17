#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_colorTexture; // Holds linear view-space Z
uniform mat4 u_projection;
uniform mat4 u_invProjection;
uniform mat4 u_view;              // NEEDED: To convert light positions to view space

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
};

#define MAX_LIGHTS 8
uniform PointLight u_lights[MAX_LIGHTS];
uniform int        u_numLights;

// Reconstruct the exact 3D View-Space position of the water from the 2D screen coordinate
vec3 reconstructViewPos(vec2 uv, float linearDepth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, -1.0, 1.0);
    vec4 viewPos = u_invProjection * clipPos;
    viewPos.xyz /= viewPos.w;
    
    float actualZ = -linearDepth;
    return viewPos.xyz * (actualZ / viewPos.z);
}

void main() {
    float viewZ = texture(u_colorTexture, TexCoords).r;
    if (viewZ == 0.0) discard;

    vec2 texelSize = 1.0 / textureSize(u_colorTexture, 0);
    vec2 offset = texelSize * 1.5; // Reduced slightly for tighter, sharper normals
    
    // 1. Sample all four neighbors
    float zRight = texture(u_colorTexture, TexCoords + vec2(offset.x, 0.0)).r;
    float zLeft  = texture(u_colorTexture, TexCoords - vec2(offset.x, 0.0)).r;
    float zUp    = texture(u_colorTexture, TexCoords + vec2(0.0, offset.y)).r;
    float zDown  = texture(u_colorTexture, TexCoords - vec2(0.0, offset.y)).r;
    
    // 2. Reconstruct 3D View-Space positions (fallback to viewZ if hitting the background)
    vec3 posCurrent = reconstructViewPos(TexCoords, viewZ);
    vec3 posRight   = reconstructViewPos(TexCoords + vec2(offset.x, 0.0), zRight != 0.0 ? zRight : viewZ);
    vec3 posLeft    = reconstructViewPos(TexCoords - vec2(offset.x, 0.0), zLeft != 0.0 ? zLeft : viewZ);
    vec3 posUp      = reconstructViewPos(TexCoords + vec2(0.0, offset.y), zUp != 0.0 ? zUp : viewZ);
    vec3 posDown    = reconstructViewPos(TexCoords - vec2(0.0, offset.y), zDown != 0.0 ? zDown : viewZ);

    // 3. Calculate bidirectional slopes
    vec3 dx1 = posRight - posCurrent;
    vec3 dx2 = posCurrent - posLeft;
    vec3 dy1 = posUp - posCurrent;
    vec3 dy2 = posCurrent - posDown;

    // --- ADAPTIVE DIFFERENCE (THE FIX) ---
    // Choose the slope with the smallest depth jump. This completely eliminates 
    // the forward-difference bias (centering your highlights) AND prevents the 
    // normals from flattening out at the water's silhouette.
    vec3 ddx = (abs(dx1.z) < abs(dx2.z)) ? dx1 : dx2;
    vec3 ddy = (abs(dy1.z) < abs(dy2.z)) ? dy1 : dy2;

    vec3 normal = normalize(cross(ddx, ddy));

    // Deep Ocean Base Color
    vec3 waterColor = vec3(0.02, 0.25, 0.65);
    vec3 ambient = waterColor * 0.4;
    vec3 finalColor = ambient;
    
    // In view space, the camera is always at (0,0,0)
    vec3 viewDir = normalize(-posCurrent);

    // --- DYNAMIC POINT LIGHTING ---
    for (int i = 0; i < u_numLights; i++) {
        // Transform light from World Space to View Space
        vec3 lightPosView = (u_view * vec4(u_lights[i].position, 1.0)).xyz;
        
        vec3 toLight = lightPosView - posCurrent;
        float dist = length(toLight);
        vec3 lightDir = toLight / dist;

        // Attenuation (inverse square law with soft constant)
        float attenuation = u_lights[i].intensity / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * waterColor * u_lights[i].color * attenuation;

        // Specular Highlight (The shiny reflection/foam)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 128.0);
        vec3 specular = vec3(spec * 1.5) * u_lights[i].color * attenuation;

        finalColor += diffuse + specular;
    }

    // Write the depth so mountains occlude the water properly
    vec4 clipPos = u_projection * vec4(0.0, 0.0, -viewZ, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;
    
    // Output the final liquid color with 85% opacity
    FragColor = vec4(finalColor, 0.85);
}