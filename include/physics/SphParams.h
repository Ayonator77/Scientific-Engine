#pragma once

struct SphParams {
    int spawn_count = 8000;
    int active_particle_count = 0;
    float smoothing_radius = 0.2f;
    float target_density = 2000.0f; // water is roughly 1000 kg/m^3
    float pressure_multiplier = 800.0f;
    float viscosity = 0.05f;
    float gravity = -9.81f;
    float collision_damping = 0.6f;
};