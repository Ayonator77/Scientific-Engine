#include "core/GraphicsContext.h"
#include <stdexcept>
#include <iostream>

GraphicsContext::GraphicsContext(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error("Failed to initialize SDL");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!m_window) throw std::runtime_error("Failed to create SDL window");

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) throw std::runtime_error("Failed to create OpenGL context");

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) throw std::runtime_error("Failed to initialize GLAD");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "--- Graphics Context Initialized ---\n"
              << "Vendor: " << glGetString(GL_VENDOR) << "\n"
              << "Renderer: " << glGetString(GL_RENDERER) << "\n"
              << "OpenGL Version: " << glGetString(GL_VERSION) << "\n"
              << "------------------------------------" << std::endl;
}

GraphicsContext::~GraphicsContext() {
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void GraphicsContext::SwapBuffers() {
    SDL_GL_SwapWindow(m_window);
}