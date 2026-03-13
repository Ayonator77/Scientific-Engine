#include "core/Scene.h"
#include <limits>

Scene::Scene(int viewportWidth, int viewportHeight) : m_camera(viewportWidth, viewportHeight) {
    m_lights.push_back(PointLight{
        .position  = {3.0f, 4.0f, 2.0f},
        .color     = {1.0f, 0.95f, 0.88f},
        .intensity = 3.0f,
        .selected  = false,
        .name      = "Key Light"
    });

    SphParams default_params;
    m_sph_solver = std::make_unique<SphSolver>(default_params);
    RegeneratePlanet();
}

void Scene::RegeneratePlanet() {
    m_planet = std::make_unique<Icosahedron>();
    m_planet->Subdivide(m_planet_params.subdivisions);
    m_planet->ApplyTerrainNoise(m_planet_params.amplitude, m_planet_params.frequency, 
                                m_planet_params.octaves, m_planet_params.sea_level, 
                                m_planet_params.seed);
}

void Scene::PickLight(float ndcX, float ndcY) {
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 proj = m_camera.GetProjectionMatrix();

    glm::vec4 rayEye = glm::inverse(proj) * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 rayOrigin = glm::vec3(glm::inverse(view)[3]);

    const float lightPickRadius = 0.18f;
    float closestT = std::numeric_limits<float>::max();
    int hitIndex = -1;

    for (int i = 0; i < static_cast<int>(m_lights.size()); i++) {
        glm::vec3 oc = rayOrigin - m_lights[i].position;
        float a = glm::dot(rayDir, rayDir);
        float b = 2.0f * glm::dot(oc, rayDir);
        float c = glm::dot(oc, oc) - lightPickRadius * lightPickRadius;
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant >= 0.0f) {
            float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
            if (t > 0.0f && t < closestT) {
                closestT = t;
                hitIndex = i;
            }
        }
    }

    for (auto& l : m_lights) l.selected = false;
    if (hitIndex >= 0) m_lights[hitIndex].selected = true;
}

void Scene::DragSelectedLight(float deltaX, float deltaY) {
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    glm::vec3 camUp = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
    const float grabSpeed = 0.008f;
    
    for (auto& l : m_lights) {
        if (!l.selected) continue;
        l.position += camRight * deltaX * grabSpeed;
        l.position += camUp * deltaY * -grabSpeed;
    }
}