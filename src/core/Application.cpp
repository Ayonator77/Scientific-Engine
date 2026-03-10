#include "Application.h"
#include "core/Camera.h"
#include "core/Input.h"
#include "core/Shader.h"
#include "geometry/Icosahedron.h"
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
// #include <glad/glad.h>
// #include <SDL2/SDL.h>

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

  // Enable relative mouse mode
  // SDL_SetRelativeMouseMode(SDL_TRUE);

  // Enable double buffering
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

  glEnable(GL_DEPTH_TEST);

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

  // Load Shader from the assets folder
  Shader planetShader("assets/shaders/planet.vert",
                      "assets/shaders/planet.frag");

  // generate planet
  Icosahedron planet_core;

  planet_core.Subdivide(5);

  planet_core.ApplyTerrainNoise(0.15f, 2.0f, 6);

  // setup camera
  Camera camera(m_width, m_height);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  SDL_Event event;
  bool isDragging = false;

  while (m_isRunning) {
    // --- 1. EVENT PUMP ---
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        m_isRunning = false;
      if (event.type == SDL_MOUSEWHEEL) {
        camera.ProcessMouseScroll((float)event.wheel.y);
      }
    }

    // --- 2. INPUT UPDATE ---
    Input::Update();

    // Check for escape key to exit
    if (Input::IsKeyHeld(SDL_SCANCODE_ESCAPE)) {
      m_isRunning = false;
    }

    // Check for left click to orbit
    if (Input::IsMouseButtonHeld(SDL_BUTTON_LEFT)) {
      // Notice how clean this is compared to tracking bools in the event loop
      camera.ProcessMouseMovement(Input::GetMouseDeltaX(),
                                  -Input::GetMouseDeltaY());
    }

    // glClearColor(0.1f, 0.15f, 0.2f, 1.0f); // color: dark blue-gray
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 3. RENDERING ---
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    planetShader.Bind();

    // The Shader class handles the uniform complexity for us
    planetShader.SetMat4("model", glm::mat4(1.0f));
    planetShader.SetMat4("view", camera.GetViewMatrix());
    planetShader.SetMat4("projection", camera.GetProjectionMatrix());

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
