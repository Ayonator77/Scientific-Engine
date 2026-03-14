#pragma once
#include "physics/Particle.h"
#include "physics/SphParams.h"
#include "core/ComputeShader.h"
#include <vector>
#include <memory>

class SphSolver{
public:
    SphSolver(const SphParams& params);
    ~SphSolver();

    void Update(float dt);
    void Reset();
    void LogGPUState();

    //Getters for the Renderer
    unsigned int GetVAO() const { return m_VAO; }
    int GetParticleCount() const { return m_params.particle_count; }
    SphParams& GetParams() { return m_params; }

private:
    void InitializeParticles();
    void SetupBuffers();

    SphParams m_params;
    std::vector<Particle> m_particles;

    unsigned int m_VAO = 0;
    unsigned int m_SSBO = 0;

    //SPH requires two distinct math passes.
    // You cannot calculate force until every particle knows its neighbor's density.
    std::unique_ptr<ComputeShader> m_compute_density_pressure;
    std::unique_ptr<ComputeShader> m_compute_forces;


};