#include "ui/Editor.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

Editor::Editor(SDL_Window* window, SDL_GLContext gl_context){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO(); 
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;        // Enable Keyboard Controls
    io.IniFilename = "imgui.ini"; // Persist ImGui settings in working directory

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 460");

    ApplyTheme();
}

Editor::~Editor(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void Editor::ProcessEvent(const SDL_Event& event){
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void Editor::BeginFrame(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void Editor::EndFrame(){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Editor::WantsCaptureKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }
bool Editor::WantsCaptureMouse() const { return ImGui::GetIO().WantCaptureMouse; }

/**
 * Main Render entry point 
*/
EditorOutput Editor::OnRender(PlanetParams& planetParams, SphParams& sphParams, std::vector<PointLight>& pointLights, float fps, int particle_count, float kinetic_energy){
    EditorOutput output;
    SetupDockspace();

    RenderPlanetPanel(planetParams, output);
    RenderLightsPanel(pointLights);
    RenderSimPanel(sphParams, output, kinetic_energy, particle_count);
    RenderStatsBar(fps, particle_count);

    return output;
}

//Dockspace - fullscreen invisible host window that everything else docks into

void Editor::SetupDockspace(){
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags = 
        ImGuiWindowFlags_NoTitleBar        |
        ImGuiWindowFlags_NoCollapse        |
        ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoMove            |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus        |
        ImGuiWindowFlags_MenuBar           |
        ImGuiWindowFlags_NoDocking;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // transparent — let 3D scene show through
    ImGui::Begin("##DockSpace Host", nullptr, host_flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    RenderMenuBar();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    //Build default layout only on the first frame
    //After that ImGui restores the layout from the ini file automatically
    if(m_first_frame){
        SetupDefaultLayout(dockspace_id);
        m_first_frame = false;
    }

    ImGui::End();
}


//Programatically creates left + right sidebar layout on first launch.
void Editor::SetupDefaultLayout(unsigned int dockspace_id){
    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any existing layout
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node with the DockSpace flag
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize); // Set size to fill the viewport

    ImGuiID left, right, center, bottom;

    //carve left sidebar (22% of total width)
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.22f, &left, &center);
    //carve right sidebar (28% remaining = ~22% of total width
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, &right, &center);
    //carve bottom stats bar (4% of remaining height)
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.04f, &bottom, &center);

    // Assign windows to regions. "Lights" and "Simulation" share the right
    // node as tabs — ImGui handles the tab bar automatically.
    ImGui::DockBuilderDockWindow("Planet Generation", left);
    ImGui::DockBuilderDockWindow("Lights", right);
    ImGui::DockBuilderDockWindow("Simulation", right);
    ImGui::DockBuilderDockWindow("Stats", bottom);
    ImGui::DockBuilderFinish(dockspace_id);
}

//Menu Bar

void Editor::RenderMenuBar(){
    if(!ImGui::BeginMenuBar()) return;

    if(ImGui::BeginMenu("File")){
        if(ImGui::MenuItem("Save Parameters", "Ctrl+S")){ /*TODO: serialize to json*/}
        if(ImGui::MenuItem("Load Parameters", "Ctrl+O")){ /*TODO: deserialize from json*/}
        ImGui::Separator();
        if(ImGui::MenuItem("Exit", "Esc")){ /*TODO: signal app to quit*/}
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("View")){ 
        if(ImGui::MenuItem("Reset Camera", "R")){/*TODO*/}
        if(ImGui::MenuItem("Toggle Wireframe", "F1")){/*TODO*/}
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Simulation")){
        if(ImGui::MenuItem("Drop Water", "Space")){/*TODO*/}
        if(ImGui::MenuItem("Reset Simulation")){/*TODO*/}
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

//Planet regeneration panel

void Editor::RenderPlanetPanel(PlanetParams& params, EditorOutput& output){
    ImGui::Begin("Planet Generation");

    ImGui::SeparatorText("Geometry");

    //Seed - int, use InputInt for precise control
    int seed = static_cast<int>(params.seed);
    if(ImGui::InputInt("Seed", &seed)){
        params.seed = static_cast<uint32_t>(std::max(0, seed)); //Prevent negative seeds
    }
    ImGui::SameLine();

    if(ImGui::Button("Random")){
        params.seed = static_cast<uint32_t>(rand());
    }

    //Subdivisions: 1-8. more  than 7 is approx. 327k triangles - warn the user
    ImGui::SliderInt("Subdivisions", &params.subdivisions, 1, 7);
    if(params.subdivisions >= 7){
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), "Warning: %d = %d triangles", params.subdivisions, 20 * (1 << (2 * params.subdivisions)));
    }

    ImGui::SeparatorText("Terrain Noise");

    ImGui::SliderFloat("Amplitude", &params.amplitude, 0.0f, 0.5f, "%.3f");
    ImGui::SliderFloat("Frequency", &params.frequency, 0.1f, 8.0f, "%.2f");
    ImGui::SliderInt("Octaves", &params.octaves, 1, 10);
    ImGui::SliderFloat("Sea Level", &params.sea_level, 0.0f, 0.3f, "%.3f");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    //Full-width generator buttonin a distinct color to draw attention
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.58f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.70f, 0.30f, 1.0f));
    if(ImGui::Button("Regenerate Planet", ImVec2(-1.0f, 0.0f))){
        output.planet_regen_requested = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}

//Lights panel
void Editor::RenderLightsPanel(std::vector<PointLight>& point_lights){
    ImGui::Begin("Lights");

    // "Add Light" button at the top
    if(ImGui::Button("+ Add Light", ImVec2(-1.0f, 0.0f))){
        PointLight new_light;
        new_light.name = "Light " + std::to_string(point_lights.size());
        //Offset new light slightly so they don't all stack
        new_light.position = {3.0f + point_lights.size() * 0.5f, 3.0f,0.0f};
        point_lights.push_back(new_light);
    }

    ImGui::Separator();
    ImGui::Spacing();

    int remove_index = -1; // Track which light to remove after the loop

    for(int i = 0; i < static_cast<int>(point_lights.size()); i++){
        ImGui::PushID(i); // Ensure unique ID for each light's controls
        bool is_selected = point_lights[i].selected;

        //Highlight the selected lights header with a tinted color
        if(is_selected){
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.35f, 0.58f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.42f, 0.68f, 0.9f));
        }

        //Prefix the label with a colored dot as a visual indicator
        std::string label = (is_selected ? "  [*] " : "   o  ") + point_lights[i].name;

        // Default-open the header if this light is selected
        ImGuiTreeNodeFlags flags = is_selected ? ImGuiTreeNodeFlags_DefaultOpen : 0;

        bool open = ImGui::CollapsingHeader(label.c_str(), flags);

        if(is_selected) ImGui::PopStyleColor(2);

        //Clicking header slects this light (and deselects others)
        if(ImGui::IsItemClicked()){
            for(auto& l : point_lights) l.selected = false;
            point_lights[i].selected = true;
        }

        if (open) {
            ImGui::Indent(8.0f);

            // Position: DragFloat3 is the most natural for 3D coordinates
            // Each unit of drag = 0.05 world units
            ImGui::DragFloat3("Position", &point_lights[i].position.x, 0.05f, -20.0f, 20.0f);

            // Color picker (compact swatch + RGB inputs)
            ImGui::ColorEdit3("Color", &point_lights[i].color.r);

            ImGui::SliderFloat("Intensity", &point_lights[i].intensity, 0.0f, 10.0f, "%.2f");

            if (is_selected) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f),
                                   "SELECTED   |   Hold G + drag mouse to move");
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.18f, 0.18f, 1.0f));
            if (ImGui::SmallButton("Delete")) {
                remove_index = i;
            }
            ImGui::PopStyleColor(2);

            ImGui::Unindent(8.0f);
        }

        ImGui::Spacing();
        ImGui::PopID();
    }

    // Perform removal outside the iteration loop to avoid iterator invalidation
    if (remove_index >= 0) {
        point_lights.erase(point_lights.begin() + remove_index);
    }

    ImGui::End();
    
}

void Editor::RenderSimPanel(SphParams& params, EditorOutput& output, float kineticEnergy, int particleCount) {
    ImGui::Begin("Simulation");

    ImGui::SeparatorText("Fluid Properties");
    ImGui::SliderInt("Spawn Count", &params.spawn_count, 1000, 60000); 
    ImGui::SliderFloat("Radius", &params.smoothing_radius, 0.05f, 0.5f, "%.3f");
    ImGui::SliderFloat("Target Density", &params.target_density, 100.0f, 2000.0f, "%.1f");
    ImGui::SliderFloat("Pressure", &params.pressure_multiplier, 5.0f, 2000.0f, "%.1f");
    ImGui::SliderFloat("Viscosity", &params.viscosity, 0.00f, 0.2f, "%.3f");
    ImGui::SliderFloat("Gravity", &params.gravity, -20.0f, 0.0f, "%.2f");
    ImGui::SliderFloat("Wall Damping", &params.collision_damping, 0.1f, 1.0f, "%.2f");

    ImGui::Spacing();
    ImGui::SeparatorText("State");
    ImGui::Text("Particles:      %d", particleCount);

    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.58f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.70f, 0.30f, 1.0f));
    if (ImGui::Button("Drop Water", ImVec2(-1.0f, 0.0f))) {
        output.sim_reset_requested = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.18f, 1.0f)); // Red button
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.24f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.30f, 0.30f, 1.0f));
    if (ImGui::Button("Log GPU State to Console", ImVec2(-1.0f, 0.0f))) {
        output.debug_log_requested = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}


// Stats bar — thin bottom panel, no title bar, just live numbers


void Editor::RenderStatsBar(float fps, int particleCount) {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f   |   Particles: %d   |   OpenGL 4.6", fps, particleCount);
    ImGui::End();
}

// Theme — a clean dark editor palette. Not the default ImGui dark.
void Editor::ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding   = 4.0f;
    s.FrameRounding    = 3.0f;
    s.ScrollbarRounding = 3.0f;
    s.GrabRounding     = 3.0f;
    s.TabRounding      = 3.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize  = 0.0f;
    s.ItemSpacing      = ImVec2(8.0f, 5.0f);
    s.FramePadding     = ImVec2(6.0f, 4.0f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.45f, 0.52f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_Border]                = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.24f, 0.32f, 1.00f);
    c[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.22f, 0.38f, 1.00f);
    c[ImGuiCol_MenuBarBg]             = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.32f, 0.40f, 1.00f);
    c[ImGuiCol_CheckMark]             = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.37f, 0.66f, 1.00f, 1.00f);
    c[ImGuiCol_Button]                = ImVec4(0.18f, 0.35f, 0.60f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.46f, 0.74f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.18f, 0.35f, 0.58f, 0.70f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.46f, 0.74f, 1.00f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.24f, 0.46f, 0.74f, 1.00f);
    c[ImGuiCol_TabActive]             = ImVec4(0.18f, 0.35f, 0.58f, 1.00f);
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.27f, 0.44f, 1.00f);
    c[ImGuiCol_DockingPreview]        = ImVec4(0.28f, 0.56f, 1.00f, 0.55f);
    c[ImGuiCol_DockingEmptyBg]        = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_Separator]             = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.28f, 0.56f, 1.00f, 0.80f);
}