#include "physics/SphSolver.h"
#include <glad/glad.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

SphSolver::SphSolver(const SphParams& params) : m_params(params) {
    // Load the compute shaders (we will create these files next)
    m_compute_density_pressure = std::make_unique<ComputeShader>("assets/shaders/sph_density.comp");
    m_compute_forces = std::make_unique<ComputeShader>("assets/shaders/sph_forces.comp");

    Reset();
}

SphSolver::~SphSolver() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_SSBO) glDeleteBuffers(1, &m_SSBO);
}

void SphSolver::Reset() {
    InitializeParticles();
    SetupBuffers();
}

void SphSolver::InitializeParticles() {
    m_particles.clear();

    //calculate how many particles fit on one side of a 3D cube
    int side = static_cast<int>(std::cbrt(m_params.particle_count));
    float spacing = m_params.smoothing_radius * 0.5f;
    float offset = (side * spacing) / 2.0f;

    for(int x = 0; x < side; ++x){
        for(int y = 0; y < side; ++y){
            for(int z = 0; z < side; ++z){
                Particle p;
                //Spawn block of water hovering at y = 3.0;
                p.position = glm::vec4( 
                    (x * spacing) - offset,
                    (y * spacing) + 3.0f,
                    (z * spacing) - offset,
                    0.0f // w component holds density
                );
                m_particles.push_back(p);
            }
        }
    }
    //Update count in case the cube root truncated the original requested ammount
    m_params.particle_count = static_cast<int>(m_particles.size());
}   

void SphSolver::SetupBuffers() {
    if (m_VAO == 0) glGenVertexArrays(1, &m_VAO);
    if (m_SSBO == 0) glGenBuffers(1, &m_SSBO);

    glBindVertexArray(m_VAO);

    // Create the SSBO and push the initial cpu grid into the gpu
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_particles.size() * sizeof(Particle), m_particles.data(), GL_DYNAMIC_DRAW);

    // Bind it to Compute Shader Binding Point 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);

    // Bind it as an Array Buffer so the Vertex Shader can draw it
    glBindBuffer(GL_ARRAY_BUFFER, m_SSBO);

    // Map the particle.position (vec4) to Layout Location 0 in the vertex shader
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

}

void SphSolver::Update(float dt){
    // Prevent physical explosions on lag spikes
    if(dt > 0.02f) dt = 0.02f;

    //Divide the gpu into workgroups of 256 threads
    unsigned int num_groups = (m_params.particle_count + 255) /256;

    // Pass 1: Density & Pressure
    m_compute_density_pressure->Bind();
    m_compute_density_pressure->SetInt("u_numParticles", m_params.particle_count);
    m_compute_density_pressure->SetFloat("u_smoothingRadius", m_params.smoothing_radius);
    m_compute_density_pressure->SetFloat("u_targetDensity", m_params.target_density);
    m_compute_density_pressure->SetFloat("u_pressureMultiplier", m_params.pressure_multiplier);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);
    m_compute_density_pressure->Dispatch(num_groups, 1, 1);
    m_compute_density_pressure->Wait(); // STOP: Wait for all cores to finish before calculating force!

    //Pass 2: Forces & Integration
    m_compute_forces->Bind();
    m_compute_forces->SetInt("u_numParticles", m_params.particle_count);
    m_compute_forces->SetFloat("u_smoothingRadius", m_params.smoothing_radius);
    m_compute_forces->SetFloat("u_viscosity", m_params.viscosity);
    m_compute_forces->SetFloat("u_gravity", m_params.gravity);
    m_compute_forces->SetFloat("u_collisionDamping", m_params.collision_damping);
    m_compute_forces->SetFloat("u_dt", dt);

    m_compute_forces->Dispatch(num_groups, 1, 1);
    m_compute_forces->Wait(); // STOP: Wait for integration before rendering
}

void SphSolver::LogGPUState() {
    // 1. Tell OpenGL we want to read the SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
    
    // 2. Lock the GPU memory and get a CPU pointer
    Particle* ptr = (Particle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    
    if (ptr) {
        float maxVel = 0.0f;
        float minDensity = 999999.0f;
        float maxDensity = -999999.0f;
        double totalKineticEnergy = 0.0;
        int nanCount = 0;

        // 3. Scan the macro state
        for (int i = 0; i < m_params.particle_count; i++) {
            float v = glm::length(glm::vec3(ptr[i].velocity));
            if (std::isnan(v)) nanCount++;
            
            maxVel = std::max(maxVel, v);
            
            float d = ptr[i].position.w;
            if (!std::isnan(d)) {
                minDensity = std::min(minDensity, d);
                maxDensity = std::max(maxDensity, d);
            }
            
            totalKineticEnergy += 0.5 * ptr[i].force.w * (v * v);
        }

        // 4. Open a text file to dump the VRAM
        std::ofstream file("gpu_vram_dump.txt");
        if (file.is_open()) {
            file << "=== SPH GPU MEMORY DIAGNOSTIC ===\n";
            file << "Total Particles: " << m_params.particle_count << "\n";
            file << "NaN (Corrupted) Particles: " << nanCount << "\n";
            file << "Max Velocity: " << maxVel << " m/s\n";
            file << "Density Range: [" << minDensity << ", " << maxDensity << "]\n";
            file << "System Kinetic Energy: " << totalKineticEnergy << " J\n";
            file << "=================================\n\n";
            
            file << "--- INDIVIDUAL PARTICLE DATA ---\n";
            file << "Format: [Index] Pos(x,y,z) | Vel(x,y,z) | Density | Pressure\n";
            
            // Dump every single particle to the file
            for (int i = 0; i < m_params.particle_count; i++) {
                file << "[" << i << "] "
                     << "P(" << ptr[i].position.x << ", " << ptr[i].position.y << ", " << ptr[i].position.z << ") | "
                     << "V(" << ptr[i].velocity.x << ", " << ptr[i].velocity.y << ", " << ptr[i].velocity.z << ") | "
                     << "D:" << ptr[i].position.w << " | "
                     << "Pr:" << ptr[i].velocity.w << "\n";
            }
            
            file.close();
            std::cout << "[SUCCESS] VRAM successfully dumped to gpu_vram_dump.txt" << std::endl;
        } else {
            std::cout << "[ERROR] Could not open file for writing." << std::endl;
        }

        // 5. CRITICAL: Unlock the memory
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    } else {
        std::cout << "[ERROR] Failed to map GPU memory.\n";
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}