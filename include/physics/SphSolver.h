#pragma once
#include "physics/Particle.h"
#include "physics/SphParams.h"
#include "core/ComputeShader.h"
#include <vector>
#include <memory>
#include <fstream>

class SphSolver{
public:
    SphSolver(const SphParams& params);
    ~SphSolver();

    void Update(float dt);
    void Reset();
    void TriggerLogging() { m_logFramesRemaining = 5; }

    //Getters for the Renderer
    unsigned int GetVAO() const { return m_VAO; }
    int GetParticleCount() const { return m_params.active_particle_count; }
    SphParams& GetParams() { return m_params; }

private:
    void InitializeParticles();
    void SetupBuffers();
    void WriteLogFrame();

    SphParams m_params;
    std::vector<Particle> m_particles;
    int m_logFramesRemaining = 0;
    std::ofstream m_logFile;

    unsigned int m_VAO = 0;
    unsigned int m_SSBO = 0;
    unsigned int m_spatial_indicesSSBO = 0; //Holds <Hash, ParticleIndex> pairs
    unsigned int m_spatial_offsetSSBO = 0; //Holds start index for each cell

    // 4-pass compute pipline
    std::unique_ptr<ComputeShader> m_compute_density_pressure;
    std::unique_ptr<ComputeShader> m_compute_forces;
    std::unique_ptr<ComputeShader> m_compute_spatial_hash;
    std::unique_ptr<ComputeShader> m_compute_bitonic_sort;
    std::unique_ptr<ComputeShader> m_compute_spatial_offsets;
};