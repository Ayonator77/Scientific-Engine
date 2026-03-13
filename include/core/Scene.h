#pragma once
#include "core/Camera.h"
#include "geometry/Icosahedron.h"
#include "core/PlanetParams.h"
#include "core/PointLight.h"
#include <vector>
#include <memory>

class Scene {
public:
    Scene(int viewport_width, int viewport_height);
    ~Scene() = default;

    void RegeneratePlanet();

    //Interaction
    void PickLight(float ndcX, float ndcY);
    void DragSelectedLight(float deltaX, float deltaY);

    // Getters
    Camera& GetCamera() { return m_camera; }
    PlanetParams& GetPlanetParams() { return m_planet_params; }
    std::vector<PointLight>& GetLights() { return m_lights; }
    std::unique_ptr<Icosahedron>& GetPlanet() { return m_planet; }


private:
    Camera m_camera;
    PlanetParams m_planet_params;
    std::vector<PointLight> m_lights;
    std::unique_ptr<Icosahedron> m_planet;

};