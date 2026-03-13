#pragma once

#include "core/Shader.h"
#include "geometry/Icosahedron.h"
#include "core/Camera.h"
#include "core/PointLight.h"
#include <vector>
#include <memory>

class Renderer{
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void Clear() const;
    void SetWireFrame(bool enabled) const;

    void DrawPlanet(const Icosahedron& planet, const Camera& camera, const std::vector<PointLight>& lights) const;
    void DrawLightBillboard(const Camera& camera, const std::vector<PointLight>& lights);

private:
    std::unique_ptr<Shader> m_planet_shader;
    std::unique_ptr<Shader> m_light_shader;
    unsigned int m_dummy_VAO = 0;
};
