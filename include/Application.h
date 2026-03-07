#pragma once


#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <string>

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

    bool m_isRunning;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;

    std::string m_title;
    int m_width;
    int m_height;
};