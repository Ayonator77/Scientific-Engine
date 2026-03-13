#pragma once


#pragma once

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <memory>

#include "ui/Editor.h"
#include "core/PlanetParams.h"
#include "core/PointLight.h"
#include "core/Input.h" 

class Application {
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    void Init();
    void Cleanup();

    // Removed the fake RaycastLights and RenderLightBillBoards stubs

    // Core State
    bool m_isRunning;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    std::string m_title;
    int m_width;
    int m_height;

    // Engine Subsystems
    Input m_input; // Instance-owned input snapshot
    std::unique_ptr<Editor> m_editor;
    
    // Engine State
    PlanetParams m_planet_params;
    std::vector<PointLight> m_lights;
    unsigned int m_dummy_VAO = 0;
};