#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>

class ComputeShader {
public:
    unsigned int ID;

    ComputeShader(const char* compute_path);
    ~ComputeShader();

    void Bind() const;
    void Dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const;
    void Wait() const;

    void SetFloat(const std::string& name, float value) const;
    void SetInt(const std::string& name, int value) const;
    void SetVec3(const std::string& name, const glm::vec3& vec) const;

private:
    void CheckCompileErrors(unsigned int shader, std::string type);
};
