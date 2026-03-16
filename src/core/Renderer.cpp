#include "core/Renderer.h"

Renderer::Renderer(int width, int height) { // Update signature
    m_planet_shader = std::make_unique<Shader>("assets/shaders/planet.vert", "assets/shaders/planet.frag");
    m_light_shader  = std::make_unique<Shader>("assets/shaders/light.vert",  "assets/shaders/light.frag");
    
    // Initialize the new SSFR shaders and FBO
    m_ssfr_depth_shader = std::make_unique<Shader>("assets/shaders/ssfr_depth.vert", "assets/shaders/ssfr_depth.frag");
    m_ssfr_debug_shader = std::make_unique<Shader>("assets/shaders/fullscreen.vert", "assets/shaders/ssfr_debug.frag");
    m_ssfr_blur_shader = std::make_unique<Shader>("assets/shaders/fullscreen.vert", "assets/shaders/ssfr_blur.frag");
    m_ssfr_blur_fbo = std::make_unique<Framebuffer>(width, height);
    m_ssfr_fbo = std::make_unique<Framebuffer>(width, height);

    glGenVertexArrays(1, &m_dummy_VAO);
}

Renderer::~Renderer() {
    if(m_dummy_VAO) glDeleteVertexArrays(1, &m_dummy_VAO);
}

void Renderer::Clear() const {
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetWireFrame(bool enabled) const {
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer:: DrawPlanet(const Icosahedron& planet, const Camera& camera, const std::vector<PointLight>& lights) const{
    m_planet_shader->Bind();
    m_planet_shader->SetMat4("model", glm::mat4(1.0f));
    m_planet_shader->SetMat4("view", camera.GetViewMatrix());
    m_planet_shader->SetMat4("projection", camera.GetProjectionMatrix());

    int num_lights = static_cast<int>(lights.size());
    m_planet_shader->SetInt("u_numLights", num_lights);

    for(int i =0; i < num_lights; i++){
        std::string base = "u_lights[" + std::to_string(i) + "].";
        m_planet_shader->SetVec3(base + "position", lights[i].position);
        m_planet_shader->SetVec3(base + "color", lights[i].color);
        m_planet_shader->SetFloat(base + "intensity", lights[i].intensity);
    }
    planet.Draw();
}

void Renderer::DrawLightBillboard(const Camera& camera, const std::vector<PointLight>& lights){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_light_shader->Bind();
    m_light_shader->SetMat4("view", camera.GetViewMatrix());
    m_light_shader->SetMat4("projection", camera.GetProjectionMatrix());

    glBindVertexArray(m_dummy_VAO);
    for(const auto& l : lights){
        m_light_shader->SetVec3("u_position", l.position);
        m_light_shader->SetVec3("u_color", l.color * l.intensity);
        m_light_shader->SetBool("u_selected", l.selected);
        m_light_shader->SetFloat("u_pointSize", l.selected ? 22.0f : 14.0f);
        glDrawArrays(GL_POINTS, 0, 1);
    }
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void Renderer::DrawParticle(unsigned int vao, int particle_count, const Camera& camera, float particle_radius) {
    if (particle_count == 0) return;

    // --- PASS 1: DRAW TO HIDDEN FRAMEBUFFER (Unchanged) ---
    m_ssfr_fbo->Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    m_ssfr_depth_shader->Bind();
    m_ssfr_depth_shader->SetMat4("view", camera.GetViewMatrix());
    m_ssfr_depth_shader->SetMat4("projection", camera.GetProjectionMatrix());
    m_ssfr_depth_shader->SetFloat("u_particleRadius", particle_radius);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particle_count);
    glBindVertexArray(0);
    m_ssfr_fbo->Unbind();

    // --- PASS 1.5: BILATERAL BLUR ---
    // We draw the raw texture onto a fullscreen quad and blur it into the 2nd FBO
    m_ssfr_blur_fbo->Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); // This is purely a 2D pass!

    m_ssfr_blur_shader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ssfr_fbo->GetColorAttachment()); // Read from Pass 1
    m_ssfr_blur_shader->SetInt("u_colorTexture", 0);

    glBindVertexArray(m_dummy_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    m_ssfr_blur_fbo->Unbind();

    // --- PASS 2: COMPOSITE ONTO SCREEN ---
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ssfr_debug_shader->Bind();
    
    // Provide both matrices to reconstruct the depth and normals
    m_ssfr_debug_shader->SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));
    m_ssfr_debug_shader->SetMat4("u_projection", camera.GetProjectionMatrix()); // <--- NEW!
    
    // Bind the BLURRED texture to Slot 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ssfr_blur_fbo->GetColorAttachment()); // <--- Read from Pass 1.5
    m_ssfr_debug_shader->SetInt("u_colorTexture", 0);

    glBindVertexArray(m_dummy_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}