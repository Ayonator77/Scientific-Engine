#include "core/Application.h"

Application::Application(const std::string& title, int width, int height)
    : m_isRunning(false), m_width(width), m_height(height) 
{
    m_context = std::make_unique<GraphicsContext>(title, width, height);
    m_renderer = std::make_unique<Renderer>(width, height);
    m_editor = std::make_unique<Editor>(m_context->GetWindow(), m_context->GetGLContext());
    m_scene = std::make_unique<Scene>(width, height);
    m_isRunning = true;
}

Application::~Application() {
    // Release resources in strict reverse-initialization order
    m_scene.reset();
    m_editor.reset();
    m_renderer.reset();
    m_context.reset();
}

void Application::ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        m_editor->ProcessEvent(event);

        if (event.type == SDL_QUIT) m_isRunning = false;

        // --- HANDLE DYNAMIC WINDOW RESIZING ---
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                m_width = event.window.data1;
                m_height = event.window.data2;
                
                glViewport(0, 0, m_width, m_height); // Tell OpenGL the new canvas size
                m_scene->GetCamera().UpdateResolution(m_width, m_height); // Fix aspect ratio
                m_renderer->Resize(m_width, m_height); // Reallocate SSFR buffers
            }
        }

        // --- FULLSCREEN TOGGLE (F11) ---
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11) {
            m_isFullscreen = !m_isFullscreen;
            SDL_SetWindowFullscreen(m_context->GetWindow(), m_isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }

        if (event.type == SDL_MOUSEWHEEL && !m_editor->WantsCaptureMouse()) {
            m_scene->GetCamera().ProcessMouseScroll(static_cast<float>(event.wheel.y));
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !m_editor->WantsCaptureMouse()) {
            float ndcX = (2.0f * event.button.x) / m_width - 1.0f;
            float ndcY = 1.0f - (2.0f * event.button.y) / m_height;
            m_scene->PickLight(ndcX, ndcY);
        }
    }

    m_input.Update();

    if (m_input.IsKeyHeld(SDL_SCANCODE_ESCAPE)) m_isRunning = false;

    bool anyLightSelected = false;
    for (const auto& l : m_scene->GetLights()) {
        if (l.selected) { anyLightSelected = true; break; }
    }

    if (anyLightSelected && m_input.IsKeyHeld(SDL_SCANCODE_G)) {
        m_scene->DragSelectedLight(m_input.GetMouseDeltaX(), m_input.GetMouseDeltaY());
    } else if (m_input.IsMouseButtonHeld(SDL_BUTTON_LEFT) && !m_editor->WantsCaptureMouse()) {
        m_scene->GetCamera().ProcessMouseMovement(m_input.GetMouseDeltaX(), -m_input.GetMouseDeltaY());
    }
}


void Application::run() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = now;
    float fps = 0.0f;

    while (m_isRunning) {
        last = now;
        now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
        fps = (dt > 0.0f) ? 1.0f / dt : fps;

        // Input Phase
        ProcessInput();

        // Physics Phase
        m_scene->GetSphSolver().Update(dt, m_scene->GetPlanetParams());;

        // Render Phase
        m_renderer->Clear();
        if (m_scene->GetPlanet()) {
            m_renderer->DrawPlanet(*m_scene->GetPlanet(), m_scene->GetCamera(), m_scene->GetLights());
        }

        m_renderer->DrawParticle(
            m_scene->GetSphSolver().GetVAO(), 
            m_scene->GetSphSolver().GetParticleCount(), 
            m_scene->GetCamera(),
            m_scene->GetSphSolver().GetParams().smoothing_radius,
            m_scene->GetLights()
        );
        m_renderer->DrawLightBillboard(m_scene->GetCamera(), m_scene->GetLights());

        // UI Phase
        m_editor->BeginFrame();
        EditorOutput out = m_editor->OnRender(m_scene->GetPlanetParams(), m_scene->GetSphSolver(), m_scene->GetLights(), fps, 0.0f);
        m_editor->EndFrame();
 
        if (out.planet_regen_requested) {
            m_scene->RegeneratePlanet();
        }

        if(out.sim_reset_requested){
            m_scene->GetSphSolver().Reset();
        }

        if(out.debug_log_requested){
            m_scene->GetSphSolver().TriggerLogging();
        }

        if (out.export_csv_requested) {
            m_scene->GetSphSolver().ExportHydrostaticCSV();
        }

        m_context->SwapBuffers();
    }
}
