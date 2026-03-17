#pragma once

#include "core/Shader.h"
#include "geometry/Icosahedron.h"
#include "core/Camera.h"
#include "core/PointLight.h"
#include "core/Framebuffer.h"
#include <vector>
#include <memory>

class Renderer{
public:
    Renderer(int width, int height);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void Clear() const;
    void SetWireFrame(bool enabled) const;

    void DrawPlanet(const Icosahedron& planet, const Camera& camera, const std::vector<PointLight>& lights) const;
    void DrawLightBillboard(const Camera& camera, const std::vector<PointLight>& lights);
    void DrawParticle(unsigned int vao, int particle_count,  const Camera& camera, float particle_radius);
    void Resize(int width, int height);

private:
    std::unique_ptr<Shader> m_planet_shader;
    std::unique_ptr<Shader> m_light_shader;
    std::unique_ptr<Shader> m_particle_shader;

    // --- SSFR PIPELINE ---
    std::unique_ptr<Framebuffer> m_ssfr_fbo;
    std::unique_ptr<Shader> m_ssfr_depth_shader;
    std::unique_ptr<Shader> m_ssfr_debug_shader;
    std::unique_ptr<Framebuffer> m_ssfr_blur_fbo;
    std::unique_ptr<Shader> m_ssfr_blur_shader;

    unsigned int m_dummy_VAO = 0;
};
