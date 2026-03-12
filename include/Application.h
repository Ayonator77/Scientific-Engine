#pragma once


#include "core/PlanetParams.h"
#include "core/PointLight.h"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <vector>

class Editor;
class Application {

public:
    Application(const std::string&  title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    void Init();
    void Cleanup();
    void RegeneratePlanet();
    void RenderLightBillBoards();
    bool RaycastLights(int mouseX, int mouseY);

    bool m_isRunning;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    std::string m_title;
    int m_width;
    int m_height;

    //Editor owns ImGui - must be destroyed before GL context is deleted
    std::unique_ptr<Editor> m_editor;

    PlanetParams m_planet_params;
    std::vector<PointLight> m_lights;

    //Dummy VAO for zero-attribute GL_Points draw (lights billboard)
    unsigned int m_dummy_VAO = 0;
};