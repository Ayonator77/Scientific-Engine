#pragma once

struct SphParams {
    int particle_count = 10000;
    float smoothing_radius = 0.2f;
    float target_density = 1000.0f; // water is roughly 1000 kg/m^3
    float pressure_multiplier = 200.0f;
    float viscosity = 0.05f;
    float gravity = -9.81f;
    float collision_damping = 0.5f;
};