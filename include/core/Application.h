#pragma once

#include <string>
#include <memory>
#include <SDL2/SDL.h>

#include "core/GraphicsContext.h"
#include "core/Scene.h"
#include "core/Renderer.h"
#include "core/Input.h"
#include "ui/Editor.h"

class Application {
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    void ProcessInput();

    bool m_isRunning;
    int m_width;
    int m_height;

    std::unique_ptr<GraphicsContext> m_context;
    Input m_input;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Editor> m_editor;
};