#pragma once
#include "core/PlanetParams.h"
#include "core/PointLight.h"
#include <SDL2/SDL.h>
#include <vector>
#include "physics/SphParams.h"
#include "physics/SphSolver.h"
/*
Struct to hold the parsed csv rows
*/
struct CsvDataPoint{
   float radius, density, pressure, velocity;
};

/**
 * Output flags that the applicaiton reads each frame to act on UI events
 * Keeps the editor completly decoupled from the engine objects 
 */
struct EditorOutput
 {
    bool planet_regen_requested = false;
    bool sim_reset_requested = false;
    bool debug_log_requested = false;
    bool export_csv_requested = false;
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
   EditorOutput OnRender(PlanetParams& planetParams, SphSolver& solver, std::vector<PointLight>& pointLights, float fps, float kinetic_energy);
   void RenderSimPanel(SphParams& params, EditorOutput& output, float kinetic_energy, int particle_count);

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
   void RenderStatsBar(float fps, int particle_count);
   void LoadCSV(const std::string& filepath);
   void RenderValidationPanel();
   void RenderMemoryProfiler(SphSolver& solver);

   bool m_first_frame = true;
   std::vector<CsvDataPoint> m_validationData;
   bool m_showValidation = false;
   bool m_showMemoryProfiler = false;
};