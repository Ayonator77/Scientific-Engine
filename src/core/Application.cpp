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
#include "ui/Editor.h"
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
  glEnable(GL_PROGRAM_POINT_SIZE); // allows gl_PointSize to be set in vertex shader

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

    // Dummy VAO — no vertex data. The light billboard vertex shader reads
    // position exclusively from uniforms, so we just need something bound.
    glGenVertexArrays(1, &m_dummy_VAO);

    // Editor must be created after the GL context exists
    m_editor = std::make_unique<Editor>(m_window, m_glContext);

    // Seed the default light list with one warm key light
    m_lights.push_back(PointLight{
        .position  = {3.0f, 4.0f, 2.0f},
        .color     = {1.0f, 0.95f, 0.88f},
        .intensity = 3.0f,
        .selected  = false,
        .name      = "Key Light"
    });

  m_isRunning = true;
}

std::unique_ptr<Icosahedron> BuildPlanet(const PlanetParams& p) {
  auto planet = std::make_unique<Icosahedron>();
  planet->Subdivide(p.subdivisions);
  planet->ApplyTerrainNoise(p.amplitude, p.frequency, p.octaves, p.sea_level, p.seed) ;
  return planet;
}


void Application::run() {

  // Load Shader from the assets folder
  auto planetShader = std::make_unique<Shader>("assets/shaders/planet.vert", "assets/shaders/planet.frag");
  auto lightShader = std::make_unique<Shader>("assets/shaders/light.vert", "assets/shaders/light.frag");

  // generate planet
  auto planet = BuildPlanet(m_planet_params);

  // setup camera
  Camera camera(m_width, m_height);

  //Timing
  Uint64 now = SDL_GetPerformanceCounter();
  Uint64 last = now;
  float fps = 0.0f;



  //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  SDL_Event event;
  bool isDragging = false;

  while (m_isRunning) {
    last = now;
    now = SDL_GetPerformanceCounter();
    float dt = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());

    fps = (dt > 0.0f) ? 1.0f/dt : fps;
    
    //Events
    while (SDL_PollEvent(&event)) {
      m_editor->ProcessEvent(event);

      if (event.type == SDL_QUIT) m_isRunning = false;
      
      //Mouse wheel only drives camera when ImGui isnt using it
      if (event.type == SDL_MOUSEWHEEL && !m_editor->WantsCaptureMouse()){
        camera.ProcessMouseScroll(static_cast<float>(event.wheel.y));
      }
            // Left click: attempt to select a light by ray-picking
            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                !m_editor->WantsCaptureMouse())
            {
                int mx = event.button.x;
                int my = event.button.y;

                // Unproject mouse into a world-space ray
                float ndcX = (2.0f * mx) / m_width  - 1.0f;
                float ndcY =  1.0f - (2.0f * my) / m_height;

                glm::mat4 view = camera.GetViewMatrix();
                glm::mat4 proj = camera.GetProjectionMatrix();

                glm::vec4 rayEye = glm::inverse(proj) * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
                rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

                glm::vec3 rayDir    = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
                glm::vec3 rayOrigin = glm::vec3(glm::inverse(view)[3]);

                const float lightPickRadius = 0.18f;
                float closestT = std::numeric_limits<float>::max();
                int   hitIndex = -1;

                for (int i = 0; i < static_cast<int>(m_lights.size()); i++) {
                    glm::vec3 oc = rayOrigin - m_lights[i].position;
                    float a = glm::dot(rayDir, rayDir);
                    float b = 2.0f * glm::dot(oc, rayDir);
                    float c = glm::dot(oc, oc) - lightPickRadius * lightPickRadius;
                    float discriminant = b * b - 4.0f * a * c;
                    if (discriminant >= 0.0f) {
                        float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
                        if (t > 0.0f && t < closestT) {
                            closestT = t;
                            hitIndex = i;
                        }
                    }
                }

                // Deselect all, then select the hit light (if any)
                for (auto& l : m_lights) l.selected = false;
                if (hitIndex >= 0) m_lights[hitIndex].selected = true;
            }
        }

        // ---- Input ----
        m_input.Update();


        if (m_input.IsKeyHeld(SDL_SCANCODE_ESCAPE))
            m_isRunning = false;

        // G + drag → move selected light in camera plane
        bool anyLightSelected = false;
        for (const auto& l : m_lights)
            if (l.selected) { anyLightSelected = true; break; }

        if (anyLightSelected && m_input.IsKeyHeld(SDL_SCANCODE_G)) {
            glm::mat4 view = camera.GetViewMatrix();
            glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
            glm::vec3 camUp    = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
            const float grabSpeed = 0.008f;
            for (auto& l : m_lights) {
                if (!l.selected) continue;
                l.position += camRight * m_input.GetMouseDeltaX()  *  grabSpeed;
                l.position += camUp    * m_input.GetMouseDeltaY()  * -grabSpeed;
            }
        } else if (m_input.IsMouseButtonHeld(SDL_BUTTON_LEFT) && !m_editor->WantsCaptureMouse()) {
            camera.ProcessMouseMovement(m_input.GetMouseDeltaX(), -m_input.GetMouseDeltaY());
        }

        // ---- Clear ----
        glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- Render planet ----
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view  = camera.GetViewMatrix();
        glm::mat4 proj  = camera.GetProjectionMatrix();

        planetShader->Bind();
        planetShader->SetMat4("model",      model);
        planetShader->SetMat4("view",       view);
        planetShader->SetMat4("projection", proj);

        // Upload point lights as a uniform array
        int numLights = static_cast<int>(m_lights.size());
        planetShader->SetInt("u_numLights", numLights);
        for (int i = 0; i < numLights; i++) {
            std::string base = "u_lights[" + std::to_string(i) + "].";
            planetShader->SetVec3 (base + "position",  m_lights[i].position);
            planetShader->SetVec3 (base + "color",     m_lights[i].color);
            planetShader->SetFloat(base + "intensity", m_lights[i].intensity);
        }

        planet->Draw();

        // ---- Render light billboards ----
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        lightShader->Bind();
        lightShader->SetMat4("view",       view);
        lightShader->SetMat4("projection", proj);

        glBindVertexArray(m_dummy_VAO);
        for (const auto& l : m_lights) {
            lightShader->SetVec3 ("u_position",  l.position);
            lightShader->SetVec3 ("u_color",     l.color * l.intensity);
            lightShader->SetBool ("u_selected",  l.selected);
            lightShader->SetFloat("u_pointSize", l.selected ? 22.0f : 14.0f);
            glDrawArrays(GL_POINTS, 0, 1);
        }
        glBindVertexArray(0);
        glDisable(GL_BLEND);

        // ---- Editor / ImGui ----
        m_editor->BeginFrame();
        EditorOutput out = m_editor->OnRender(m_planet_params, m_lights, fps, 0, 0.0f);
        m_editor->EndFrame();

        // Act on editor output
        if (out.planet_regen_requested) {
            planet = BuildPlanet(m_planet_params);
        }

        SDL_GL_SwapWindow(m_window);
    }

    // planet, planetShader, billboardShader destroyed here —
    // GL context still valid, so GPU resources release cleanly.
}
void Application::Cleanup() {
    // Reset Editor first — it shuts down ImGui's GL backend
    m_editor.reset();

    if (m_dummy_VAO) {
        glDeleteVertexArrays(1, &m_dummy_VAO);
        m_dummy_VAO = 0;
    }

    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window)    SDL_DestroyWindow(m_window);
    SDL_Quit();
}
