#pragma once
#include <cstdint>

/**
 * Plain data struct to hold planet generation parameters
 * Passed by reference through the editor so UI sliders directly mutate the live value
 * Application owns canonical instance
 */
struct PlanetParams{
    uint32_t seed  = 42;
    int subdivisions = 6;
    float amplitude = 0.1f;
    float frequency = 1.26f;
    int octaves = 6;
    float sea_level = 0.03f; 
};