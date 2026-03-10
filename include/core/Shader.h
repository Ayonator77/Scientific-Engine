
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  // Program ID
  unsigned int ID;

  // Constructor reads and builds the shader
  Shader(const char *vertex_path, const char *fragment_path);
  ~Shader();

  // use/activate shader
  void Bind() const;

  // Utility uniform functions
  void SetBool(const std::string &name, bool value) const;
  void SetInt(const std::string &name, int value) const;
  void SetFloat(const std::string &name, float value) const;
  void SetMat4(const std::string &name, const glm::mat4 &mat) const;
  void SetVec3(const std::string &name, const glm::vec3 &vec) const;

private:
  void CheckCompileErrors(unsigned int shader, std::string type);
};
