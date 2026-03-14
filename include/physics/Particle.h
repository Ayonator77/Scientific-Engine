#pragma once
#include <glm/glm.hpp>

struct Particle {
    glm::vec4 position;  // xyz = postion, w = density
    glm::vec4 velocity;  // xyz = velocity, w = pressure
    glm::vec4 force;     // xyz = force,  w = mass

    Particle()
        : position(0.0f, 0.0f, 0.0f, 1.0f)
        , velocity(0.0f, 0.0f, 0.0f, 0.0f)
        , force(0.0f, 0.0f, 0.0f, 1.0f) {}

};