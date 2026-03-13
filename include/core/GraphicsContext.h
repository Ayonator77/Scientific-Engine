#pragma once
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <string>

class GraphicsContext {
public:
    GraphicsContext(const std::string& title, int width, int height);
    ~GraphicsContext();

    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    void SwapBuffers();
    SDL_Window* GetWindow() const {return m_window;}
    SDL_GLContext GetGLContext() const {return m_glContext;}

private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
};