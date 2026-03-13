#pragma once
#include "core/PlanetParams.h"
#include "core/PointLight.h"
#include <SDL2/SDL.h>
#include <vector>

/**
 * Output flags that the applicaiton reads each frame to act on UI events
 * Keeps the editor completly decoupled from the engine objects 
 */

 struct EditorOutput
 {
    bool planet_regen_requested = false;
 };
 

 /**
  * Owns full logic of ImGui lifecycle (init, frame, shutdown)
  * All panel logic is private to keep public API clean and minimal
  */

class Editor{
public:
   Editor(SDL_Window* window, SDL_GLContext gl_context);
   ~Editor();

   Editor(const Editor&) = delete; 
   Editor& operator=(const Editor&) = delete; 

   //Feed SDL events into ImGui
   void ProcessEvent(const SDL_Event& event);

   void BeginFrame();
   void EndFrame();

   //Build all panels. return flags for the application to act on
   EditorOutput OnRender(PlanetParams& planetParams, std::vector<PointLight>& pointLights, float fps, int particle_count, float kinetic_energy);

   //Query ImGui's capture state - suppress camera/game input when true
   bool WantsCaptureMouse() const;
   bool WantsCaptureKeyboard() const;

private:
   void SetupDockspace();
   void SetupDefaultLayout(unsigned int dockspace_id);
   void ApplyTheme();

   void RenderMenuBar();
   void RenderPlanetPanel(PlanetParams& planetParams, EditorOutput& output);
   void RenderLightsPanel(std::vector<PointLight>& pointLights);
   void RenderSimPanel(float kinetic_energy, int particle_count);
   void RenderStatsBar(float fps, int particle_count);

   bool m_first_frame = true;

};