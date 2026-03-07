#include "Application.h"
#include "geometry/Icosahedron.h"
#include <iostream>
#include <stdexcept>
// #include <glad/glad.h>
// #include <SDL2/SDL.h>

// --- TEMPORARY HARDCODED SHADERS ---
const char *vertexShaderSource =
    "#version 460 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos * 0.5, 1.0);\n" // Scale it down by 0.5 so it
                                                // fits in the window
    "}\n";

const char *fragmentShaderSource =
    "#version 460 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(0.0f, 1.0f, 0.5f, 1.0f);\n" // Seafoam green
    "}\n";

Application::Application(const std::string &title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_isRunning(false),
      m_window(nullptr), m_glContext(nullptr) {
  Init();
}

Application::~Application() { Cleanup(); }

void Application::Init() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error("Failed to initialize SDL");
  }
  // ENFORCE OPENGL 4.6 CORE PROFILE
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Enable double buffering
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  m_window = SDL_CreateWindow(m_title.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, m_width, m_height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  if (!m_window) {
    throw std::runtime_error("Failed to create SDL window");
  }

  m_glContext = SDL_GL_CreateContext(m_window);

  if (!m_glContext) {
    throw std::runtime_error("Failed to create OpenGL context");
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    throw std::runtime_error("Failed to initialize GLAD");
  }

  // Output the specific graphics hardware and OpenGL version to terminal
  const GLubyte *vendor = glGetString(GL_VENDOR);
  const GLubyte *renderer = glGetString(GL_RENDERER);
  const GLubyte *version = glGetString(GL_VERSION);

  std::cout << "--- Engine Initialized ---" << std::endl;
  std::cout << "Vendor: "
            << (vendor ? reinterpret_cast<const char *>(vendor)
                       : "UNKNOWN (Context Error)")
            << std::endl;
  std::cout << "Renderer: "
            << (renderer ? reinterpret_cast<const char *>(renderer)
                         : "UNKNOWN (Context Error)")
            << std::endl;
  std::cout << "OpenGL Version: "
            << (version ? reinterpret_cast<const char *>(version)
                        : "UNKNOWN (Context Error)")
            << std::endl;
  std::cout << "--------------------------" << std::endl;

  m_isRunning = true;
}

void Application::run() {

  // --- COMPILE SHADERS BEFORE THE LOOP ---
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  // ---------------------------------------

  Icosahedron planet_core;

  planet_core.Subdivide(5);

  planet_core.ApplyTerrainNoise(0.15f, 2.0f, 6);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  SDL_Event event;

  while (m_isRunning) {
    // 1. Handle input

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        m_isRunning = false;
      }
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        m_isRunning = false;
      }
    }

    glClearColor(0.1f, 0.15f, 0.2f, 1.0f); // color: dark blue-gray
    glClear(GL_COLOR_BUFFER_BIT);

    // Bind shader and issue draw command
    glUseProgram(shaderProgram);
    planet_core.Draw();

    // Swap buffers to display the rendered frame
    SDL_GL_SwapWindow(m_window);
  }
}

void Application::Cleanup() {
  if (m_glContext) {
    SDL_GL_DeleteContext(m_glContext);
  }
  if (m_window) {
    SDL_DestroyWindow(m_window);
  }
  SDL_Quit();
}
