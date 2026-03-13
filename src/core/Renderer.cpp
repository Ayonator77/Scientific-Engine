#include "core/Renderer.h"

Renderer::Renderer() {
    m_planet_shader = std::make_unique<Shader>("assets/shaders/planet.vert", "assets/shaders/planet.frag");
    m_light_shader  = std::make_unique<Shader>("assets/shaders/light.vert",  "assets/shaders/light.frag");
    m_particle_shader = std::make_unique<Shader>("assets/shaders/particle.vert", "assets/shaders/particle.frag");

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

void Renderer::DrawParticle(unsigned int vao, int particle_count, const Camera& camera){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_particle_shader->Bind();
    m_particle_shader->SetMat4("view", camera.GetViewMatrix());
    m_particle_shader->SetMat4("projection", camera.GetProjectionMatrix());

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particle_count);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}