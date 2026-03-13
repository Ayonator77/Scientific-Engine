#version 460 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
};

#define MAX_LIGHTS 8
uniform PointLight u_lights[MAX_LIGHTS];
uniform int        u_numLights;

void main() {
    vec3 norm        = normalize(Normal);
    vec3 objectColor = vec3(0.22, 0.58, 0.28);

    // Small ambient so the dark side is never pure black
    vec3 result = vec3(0.00) * objectColor;

    for (int i = 0; i < u_numLights; i++) {
        vec3  toLight     = u_lights[i].position - FragPos;
        float dist        = length(toLight);
        vec3  lightDir    = toLight / dist;

        // Inverse-square attenuation with a soft constant term
        float attenuation = u_lights[i].intensity /
                            (1.0 + 0.09 * dist + 0.032 * dist * dist);

        float diff   = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * u_lights[i].color * attenuation;

        result += diffuse * objectColor;
    }

    FragColor = vec4(result, 1.0);
}
