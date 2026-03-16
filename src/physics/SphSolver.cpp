#include "physics/SphSolver.h"
#include <glad/glad.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

SphSolver::SphSolver(const SphParams& params) : m_params(params) {
    m_compute_spatial_hash = std::make_unique<ComputeShader>("assets/shaders/sph_hash.comp");
    m_compute_bitonic_sort = std::make_unique<ComputeShader>("assets/shaders/sph_sort.comp");
    m_compute_spatial_offsets = std::make_unique<ComputeShader>("assets/shaders/sph_offsets.comp");
    m_compute_density_pressure = std::make_unique<ComputeShader>("assets/shaders/sph_density.comp");
    m_compute_forces = std::make_unique<ComputeShader>("assets/shaders/sph_forces.comp");
    // Reset();
}

SphSolver::~SphSolver() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_SSBO) glDeleteBuffers(1, &m_SSBO);
    if (m_spatial_indicesSSBO) glDeleteBuffers(1, &m_spatial_indicesSSBO);
    if (m_spatial_offsetSSBO) glDeleteBuffers(1, &m_spatial_offsetSSBO);
}

void SphSolver::Reset() {
    m_isSpawned = true;
    InitializeParticles();
    SetupBuffers();
}

void SphSolver::InitializeParticles() {
    m_particles.clear();

    //calculate how many particles fit on one side of a 3D cube
    int side = static_cast<int>(std::cbrt(m_params.spawn_count));
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
                //p.force.w = 0.05f;
                m_particles.push_back(p);
            }
        }
    }
    //Update count in case the cube root truncated the original requested ammount
    m_params.active_particle_count = static_cast<int>(m_particles.size());
}   

void SphSolver::SetupBuffers() {
// 1. Generate all buffers
    if (m_VAO == 0) glGenVertexArrays(1, &m_VAO);
    if (m_SSBO == 0) glGenBuffers(1, &m_SSBO);
    if (m_spatial_indicesSSBO == 0) glGenBuffers(1, &m_spatial_indicesSSBO); 
    if (m_spatial_offsetSSBO == 0) glGenBuffers(1, &m_spatial_offsetSSBO);

    glBindVertexArray(m_VAO);

    // 2. The Main Particle Buffer (Binding 0)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_particles.size() * sizeof(Particle), m_particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);

    // 3. The Spatial Indices Buffer (Binding 1)
    // Needs to hold a uvec2 (8 bytes) for every particle: [Hash, OriginalIndex]
    unsigned int sort_capacity = 65536; 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spatial_indicesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sort_capacity * sizeof(unsigned int) * 2, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_spatial_indicesSSBO);

    // 4. The Spatial Offsets Buffer (Binding 2)
    // Needs to hold one uint (4 bytes) for every grid cell in the universe
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spatial_offsetSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_params.spatial_grid_size * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_spatial_offsetSSBO);

    // 5. Setup the VAO for the Renderer
    glBindBuffer(GL_ARRAY_BUFFER, m_SSBO);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

}

void SphSolver::Update(float dt, const PlanetParams& planetParams) {
    if (!m_isSpawned) return;

    if (dt > 0.016f) dt = 0.016f; 

    // PASS 0 - SPATIAL HASHING & SORTING 
    unsigned int sort_capacity = 65536; 
    unsigned int hash_groups = (sort_capacity + 255) / 256;

    // 0A: Hash Pass
    m_compute_spatial_hash->Bind();
    m_compute_spatial_hash->SetInt("u_numParticles", m_params.active_particle_count);
    m_compute_spatial_hash->SetFloat("u_smoothingRadius", m_params.smoothing_radius);
    m_compute_spatial_hash->SetInt("u_tableSize", m_params.spatial_grid_size);
    m_compute_spatial_hash->Dispatch(hash_groups, 1, 1);
    m_compute_spatial_hash->Wait();

    // 0B: Bitonic Sort Pass
    m_compute_bitonic_sort->Bind();
    m_compute_bitonic_sort->SetInt("u_numElements", sort_capacity);
    
    // The recursive CPU dispatch loop required to execute a Bitonic Sort on the GPU
    for (unsigned int k = 2; k <= sort_capacity; k <<= 1) {
        for (unsigned int j = k >> 1; j > 0; j >>= 1) {
            m_compute_bitonic_sort->SetInt("u_k", k);
            m_compute_bitonic_sort->SetInt("u_j", j);
            m_compute_bitonic_sort->Dispatch(hash_groups, 1, 1);
            m_compute_bitonic_sort->Wait();
        }
    }

    // Sub-stepping: slice the frame into 2 smaller, highly stable physics steps
    const int SUB_STEPS = 2;
    float sub_dt = dt / static_cast<float>(SUB_STEPS);
    // 0C: Clear the Offsets Buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spatial_offsetSSBO);
    unsigned int clearVal = 0xFFFFFFFF; // 0xFFFFFFFF means "Empty Cell"
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearVal);

    // The GPU ONLY reads the active, perfectly cubic particle count
    unsigned int num_groups = (m_params.active_particle_count + 255) / 256;
    
    // 0D: Offset Pass
    m_compute_spatial_offsets->Bind();
    m_compute_spatial_offsets->SetInt("u_numParticles", m_params.active_particle_count);
    m_compute_spatial_offsets->Dispatch(num_groups, 1, 1);
    m_compute_spatial_offsets->Wait();
    

    for (int step = 0; step < SUB_STEPS; step++) {
        // ----- PASS 1: Density & Pressure -----
        m_compute_density_pressure->Bind();
        m_compute_density_pressure->SetInt("u_tableSize", m_params.spatial_grid_size);
        m_compute_density_pressure->SetInt("u_numParticles", m_params.active_particle_count);
        m_compute_density_pressure->SetFloat("u_smoothingRadius", m_params.smoothing_radius);
        m_compute_density_pressure->SetFloat("u_targetDensity", m_params.target_density);
        m_compute_density_pressure->SetFloat("u_pressureMultiplier", m_params.pressure_multiplier);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);
        m_compute_density_pressure->Dispatch(num_groups, 1, 1);
        m_compute_density_pressure->Wait();

        // ----- PASS 2: Forces & Integration -----
        m_compute_forces->Bind();
        m_compute_forces->SetInt("u_tableSize", m_params.spatial_grid_size);
        m_compute_forces->SetInt("u_numParticles", m_params.active_particle_count);
        m_compute_forces->SetFloat("u_smoothingRadius", m_params.smoothing_radius);
        m_compute_forces->SetFloat("u_viscosity", m_params.viscosity);
        m_compute_forces->SetFloat("u_gravity", m_params.gravity);
        m_compute_forces->SetFloat("u_collisionDamping", m_params.collision_damping);
        m_compute_forces->SetFloat("u_dt", sub_dt); // Pass the micro-dt!

        // ---- PASS 3: Planet Parameters ----
        m_compute_forces->SetFloat("u_planetAmplitude", planetParams.amplitude);
        m_compute_forces->SetFloat("u_planetFrequency", planetParams.frequency);
        m_compute_forces->SetInt("u_planetOctaves", planetParams.octaves);
        m_compute_forces->SetFloat("u_planetSeaLevel", planetParams.sea_level);
        m_compute_forces->SetInt("u_planetSeed", planetParams.seed);

        m_compute_forces->Dispatch(num_groups, 1, 1);
        m_compute_forces->Wait();
    }

    if (m_logFramesRemaining > 0) {
        WriteLogFrame();
    }
}

void SphSolver::WriteLogFrame() {
    // If this is the first frame of the capture, open the file and write the parameters
    if (m_logFramesRemaining == 5) {
        m_logFile.open("gpu_vram_multiframe_dump.txt");
        m_logFile << "=== SPH MULTI-FRAME DIAGNOSTIC ===\n";
        m_logFile << "Active Particles:    " << m_params.active_particle_count << "\n";
        m_logFile << "Smoothing Radius:    " << m_params.smoothing_radius << " (Sphere of Influence)\n";
        m_logFile << "Target Density:      " << m_params.target_density << "\n";
        m_logFile << "Pressure Multiplier: " << m_params.pressure_multiplier << "\n";
        m_logFile << "==================================\n\n";
        std::cout << "[LOGGING] Starting 5-frame capture..." << std::endl;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
    Particle* ptr = (Particle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    
    if (ptr) {
        float maxVel = 0.0f;
        float minDensity = 999999.0f;
        float maxDensity = -999999.0f;
        
        for (int i = 0; i < m_params.active_particle_count; i++) {
            float v = glm::length(glm::vec3(ptr[i].velocity));
            maxVel = std::max(maxVel, v);
            
            float d = ptr[i].position.w;
            if (!std::isnan(d)) {
                minDensity = std::min(minDensity, d);
                maxDensity = std::max(maxDensity, d);
            }
        }

        m_logFile << "--- FRAME " << (6 - m_logFramesRemaining) << " ---\n";
        m_logFile << "Max Velocity: " << maxVel << " m/s\n";
        m_logFile << "Density Range: [" << minDensity << ", " << maxDensity << "]\n";
        
        // Dump a sample set of particles to track their specific movement across frames
        for (int i = 0; i < 15; i++) {
            m_logFile << "[" << i << "] "
                      << "P(" << ptr[i].position.x << ", " << ptr[i].position.y << ", " << ptr[i].position.z << ") | "
                      << "D:" << ptr[i].position.w << " | "
                      << "Pr:" << ptr[i].velocity.w << "\n";
        }
        m_logFile << "\n";
        
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Decrement the counter
    m_logFramesRemaining--;
    
    // Close the file if the capture is complete
    if (m_logFramesRemaining <= 0) {
        m_logFile.close();
        std::cout << "[LOGGING] Multi-frame capture complete! Saved to gpu_vram_multiframe_dump.txt" << std::endl;
    }
}