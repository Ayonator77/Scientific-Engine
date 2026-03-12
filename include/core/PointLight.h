#pragma once
#include <glm/glm.hpp>
#include <string>

/**
 * Represents a movable point light in the scene
 * 'selected' is a runtime state - toggled by 3D ray-pick or UI panel click
 */
struct PointLight{
    glm::vec3 position = {3.0f, 3.0f, 3.0f};
    glm::vec3 color = {1.0f, 0.95f, 0.88f}; // Warm white light
    float intensity = 2.5f;
    bool selected = false; // Not serialized, runtime state only 
    std::string name = "Light"; // For UI display
};