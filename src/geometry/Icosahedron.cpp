#include "geometry/Icosahedron.h"
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>

Icosahedron::Icosahedron() : m_VAO(0), m_VBO(0), m_EBO(0), m_normalVBO(0) {
  GenerateBaseGeometry();
  CalculateNormals();
  SetupBuffers();
}

Icosahedron::~Icosahedron() {
  if (m_VAO)
    glDeleteVertexArrays(1, &m_VAO);
  if (m_VBO)
    glDeleteBuffers(1, &m_VBO);
  if (m_EBO)
    glDeleteBuffers(1, &m_EBO);
  if (m_normalVBO)
    glDeleteBuffers(1, &m_normalVBO);
}

void Icosahedron::GenerateBaseGeometry() {
  const float radius = 1.0f;
  const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

  const glm::vec3 baseVertices[12] = {
      {-1.0f, phi, 0.0f},  {1.0f, phi, 0.0f},   {-1.0f, -phi, 0.0f},
      {1.0f, -phi, 0.0f},  {0.0f, -1.0f, phi},  {0.0f, 1.0f, phi},
      {0.0f, -1.0f, -phi}, {0.0f, 1.0f, -phi},  {phi, 0.0f, -1.0f},
      {phi, 0.0f, 1.0f},   {-phi, 0.0f, -1.0f}, {-phi, 0.0f, 1.0f}};

  constexpr unsigned int baseIndices[60] = {
      0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
      4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
      6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1};

  m_vertices.reserve(12);
  m_indices.reserve(60);

  for (int i = 0; i < 12; ++i) {
    m_vertices.push_back(glm::normalize(baseVertices[i]) * radius);
  }
  m_indices.assign(baseIndices, baseIndices + 60);
}

unsigned int Icosahedron::GetMiddlePoint(
    unsigned int p1, unsigned int p2,
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> &cache) {
  bool firstIsSmaller = p1 < p2;
  unsigned int smallerIndex = firstIsSmaller ? p1 : p2;
  unsigned int greaterIndex = firstIsSmaller ? p2 : p1;
  std::pair<unsigned int, unsigned int> edge(smallerIndex, greaterIndex);

  if (cache.find(edge) != cache.end()) {
    return cache[edge];
  }

  glm::vec3 middle = glm::normalize((m_vertices[p1] + m_vertices[p2]) / 2.0f);
  unsigned int index = static_cast<unsigned int>(m_vertices.size());
  m_vertices.push_back(middle);
  cache[edge] = index;
  return index;
}

void Icosahedron::Subdivide(int subdivisions) {
  for (int i = 0; i < subdivisions; ++i) {
    std::vector<unsigned int> newIndices;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> edgeCache;

    for (size_t j = 0; j < m_indices.size(); j += 3) {
      unsigned int v1 = m_indices[j];
      unsigned int v2 = m_indices[j + 1];
      unsigned int v3 = m_indices[j + 2];

      unsigned int a = GetMiddlePoint(v1, v2, edgeCache);
      unsigned int b = GetMiddlePoint(v2, v3, edgeCache);
      unsigned int c = GetMiddlePoint(v3, v1, edgeCache);

      newIndices.insert(newIndices.end(), {v1, a, c});
      newIndices.insert(newIndices.end(), {v2, b, a});
      newIndices.insert(newIndices.end(), {v3, c, b});
      newIndices.insert(newIndices.end(), {a, b, c});
    }
    m_indices = newIndices;
  }
  CalculateNormals();
  SetupBuffers();
}

// seed offsets the noise sample space so the same frequency/amplitude
// settings produce a different terrain shape per seed value.
void Icosahedron::ApplyTerrainNoise(float amplitude, float frequency, int octaves, float seaLevel, uint32_t seed){
    // Deterministic but varied offset per seed — prime multipliers spread
    // the offset unevenly across each axis to avoid axis-aligned artifacts.
    glm::vec3 seedOffset(
        static_cast<float>(seed) * 0.1270f,
        static_cast<float>(seed) * 0.3117f,
        static_cast<float>(seed) * 0.7431f
    );

    for (auto& vertex : m_vertices) {
        glm::vec3 direction = glm::normalize(vertex);
        float elevation    = 0.0f;
        float currentFreq  = frequency;
        float currentAmp   = amplitude;
        float weight       = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            glm::vec3 samplePoint = direction * currentFreq + seedOffset;
            float rawNoise = glm::simplex(samplePoint);
            float ridge    = 1.0f - std::abs(rawNoise);
            ridge         *= ridge;
            ridge         *= weight;
            weight         = ridge;
            elevation     += ridge * currentAmp;
            currentFreq   *= 2.0f;
            currentAmp    *= 0.5f;
        }

        if (elevation < seaLevel) elevation = seaLevel;
        vertex = direction * (1.0f + elevation);
    }

    CalculateNormals();
    SetupBuffers();
}

void Icosahedron::CalculateNormals() {
    m_normals.assign(m_vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < m_indices.size(); i += 3) {
        unsigned int i1 = m_indices[i];
        unsigned int i2 = m_indices[i + 1];
        unsigned int i3 = m_indices[i + 2];

        glm::vec3 edge1 = m_vertices[i2] - m_vertices[i1];
        glm::vec3 edge2 = m_vertices[i3] - m_vertices[i1];
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        m_normals[i1] += faceNormal;
        m_normals[i2] += faceNormal;
        m_normals[i3] += faceNormal;
    }

    for (auto& n : m_normals) n = glm::normalize(n);
}


void Icosahedron::SetupBuffers() {
  if (m_VAO == 0) {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_normalVBO);
    glGenBuffers(1, &m_EBO);
  }

  glBindVertexArray(m_VAO);

  // Positions (Location 0)
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3),
               m_vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(0);

  // Normals (Location 1)
  glBindBuffer(GL_ARRAY_BUFFER, m_normalVBO);
  glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(glm::vec3),
               m_normals.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(1);

  // Indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
               m_indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void Icosahedron::Draw() const {
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
