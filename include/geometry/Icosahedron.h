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

  // core procedural generation functions
  void Subdivide(int subdivisions);
  void ApplyTerrainNoise(float amplitude, float frequency, int octaves);

  void Draw() const;

private:
  void GenerateBaseGeometry();
  void SetupBuffers();

  unsigned int GetMiddlePoint(
      unsigned int p1, unsigned int p2,
      std::map<std::pair<unsigned int, unsigned int>, unsigned int> &cache);

  std::vector<glm::vec3> m_vertices;
  std::vector<unsigned int> m_indices;

  unsigned int m_VAO, m_VBO, m_EBO;
};
