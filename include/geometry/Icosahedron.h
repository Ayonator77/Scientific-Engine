#pragma once

#include <glm/glm.hpp>
#include <map>
#include <utility>
#include <vector>

class Icosahedron {
public:
  Icosahedron();
  ~Icosahedron();

  Icosahedron(const Icosahedron &) = delete;
  Icosahedron &operator=(const Icosahedron &) = delete;

  void Subdivide(int subdivisions);
  void ApplyTerrainNoise(float amplitude, float frequency, int octaves,
                           float seaLevel = 0.05f, uint32_t seed = 0);

  void Draw() const;
  const std::vector<glm::vec3>& GetVertices() const { return m_vertices; }
  const std::vector<unsigned int>& GetIndices() const { return m_indices; }

private:
  void GenerateBaseGeometry();
  void SetupBuffers();
  void CalculateNormals();

  unsigned int GetMiddlePoint(
      unsigned int p1, unsigned int p2,
      std::map<std::pair<unsigned int, unsigned int>, unsigned int> &cache);

  std::vector<glm::vec3> m_vertices;
  std::vector<glm::vec3> m_normals;
  std::vector<unsigned int> m_indices;

  unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0, m_normalVBO = 0;
};
