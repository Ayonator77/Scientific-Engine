#include "geometry/Icosahedron.h"
#include <cmath>
#include <glad/glad.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/noise.hpp>

Icosahedron::Icosahedron() : m_VAO(0), m_VBO(0), m_EBO(0) {
  GenerateBaseGeometry();
  SetupBuffers();
}

Icosahedron::~Icosahedron() {
  if (m_VAO)
    glDeleteVertexArrays(1, &m_VAO);
  if (m_VBO)
    glDeleteBuffers(1, &m_VBO);
  if (m_EBO)
    glDeleteBuffers(1, &m_EBO);
}

void Icosahedron::GenerateBaseGeometry() {
  const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
  m_vertices = {glm::normalize(glm::vec3(-1.0f, phi, 0.0f)),
                glm::normalize(glm::vec3(1.0f, phi, 0.0f)),
                glm::normalize(glm::vec3(-1.0f, -phi, 0.0f)),
                glm::normalize(glm::vec3(1.0f, -phi, 0.0f)),
                glm::normalize(glm::vec3(0.0f, -1.0f, phi)),
                glm::normalize(glm::vec3(0.0f, 1.0f, phi)),
                glm::normalize(glm::vec3(0.0f, -1.0f, -phi)),
                glm::normalize(glm::vec3(0.0f, 1.0f, -phi)),
                glm::normalize(glm::vec3(phi, 0.0f, -1.0f)),
                glm::normalize(glm::vec3(phi, 0.0f, 1.0f)),
                glm::normalize(glm::vec3(-phi, 0.0f, -1.0f)),
                glm::normalize(glm::vec3(-phi, 0.0f, 1.0f))};

  m_indices = {0, 11, 5, 0, 5,  1,  0,  1,  7,  0,  7, 10, 0, 10, 11,
               1, 5,  9, 5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1,  8,
               3, 9,  4, 3, 4,  2,  3,  2,  6,  3,  6, 8,  3, 8,  9,
               4, 9,  5, 2, 4,  11, 6,  2,  10, 8,  6, 7,  9, 8,  1};
}

unsigned int Icosahedron::GetMiddlePoint(
    unsigned int p1, unsigned int p2,
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> &cache) {

  // Ensure the key is always ordered the same way to catch duplicates
  bool first_is_smaller = p1 < p2;
  unsigned int smaller_index = first_is_smaller ? p1 : p2;
  unsigned int greater_index = first_is_smaller ? p2 : p1;

  std::pair<unsigned int, unsigned int> edge(smaller_index, greater_index);

  // if we already split this edge, return the cached vertex index
  if (cache.find(edge) != cache.end()) {
    return cache[edge];
  }

  // otherwise calculate the new vertex, push it to the radius 1.0, and cahce in
  glm::vec3 middle = glm::normalize((m_vertices[p1] + m_vertices[p2]) / 2.0f);
  unsigned int index = static_cast<unsigned int>(m_vertices.size());

  m_vertices.push_back(middle);
  cache[edge] = index;

  return index;
}

void Icosahedron::Subdivide(int subdivisions) {
  for (int i = 0; i < subdivisions; ++i) {
    std::vector<unsigned int> new_indices;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> edge_cache;

    // iterate through all current triangles

    for (size_t j = 0; j < m_indices.size(); j += 3) {
      unsigned int v1 = m_indices[j];
      unsigned int v2 = m_indices[j + 1];
      unsigned int v3 = m_indices[j + 2];

      // Get the 3 midpoints (creating new veritices or fetching from the cache)
      unsigned int a = GetMiddlePoint(v1, v2, edge_cache);
      unsigned int b = GetMiddlePoint(v2, v3, edge_cache);
      unsigned int c = GetMiddlePoint(v3, v1, edge_cache);

      // create 3 new triangles out of the old one
      new_indices.insert(new_indices.end(), {v1, a, c});
      new_indices.insert(new_indices.end(), {v2, b, a});
      new_indices.insert(new_indices.end(), {v3, c, b});
      new_indices.insert(new_indices.end(), {a, b, c});
    }

    m_indices = new_indices; // replace old face with new high res face
  }
  SetupBuffers(); // Reload to GPU
}

void Icosahedron::ApplyTerrainNoise(float amplitude, float frequency,
                                    int octaves) {
  for (auto &vertex : m_vertices) {
    glm::vec3 direction = glm::normalize(vertex);

    float elevation = 0.0f;
    float currentFreq = frequency;
    float currentAmp = amplitude;
    float weight = 1.0f; // Used to make valleys flatter and peaks sharper

    // Advanced Ridged Multifractal Loop
    for (int i = 0; i < octaves; ++i) {
      // 1. Get the raw noise (-1.0 to 1.0)
      float rawNoise = glm::simplex(direction * currentFreq);

      // 2. Convert to a sharp ridge (0.0 to 1.0)
      float ridge = 1.0f - std::abs(rawNoise);

      // 3. Square the ridge to make the peaks sharper and valleys wider
      ridge *= ridge;

      // 4. Multiply by weight. High peaks stay sharp, low valleys stay flat.
      ridge *= weight;
      weight =
          ridge; // The next octave's weight depends on this octave's height

      // 5. Add to total elevation
      elevation += ridge * currentAmp;

      // 6. Iterate fBm properties
      currentFreq *= 2.0f; // Lacunarity
      currentAmp *= 0.5f;  // Gain
    }

    // --- THE OCEAN FLOOR CLAMP ---
    // We push the terrain out, but we set a minimum "sea level".
    // Anything below 0.05 becomes a perfectly flat ocean floor.
    float seaLevel = 0.05f;
    if (elevation < seaLevel) {
      elevation = seaLevel;
    }

    // Extrude the vertex outwards
    vertex = direction * (1.0f + elevation);
  }

  SetupBuffers(); // Re-upload to GPU
}

void Icosahedron::SetupBuffers() {
  // Only generate hardware buffers if they dont exist to prevent memory leak

  if (m_VAO == 0) {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
  }

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3),
               m_vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
               m_indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

void Icosahedron::Draw() const {
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
