#include "core/ComputeShader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

ComputeShader::ComputeShader(const char* compute_path) {
    std::string compute_code;
    std::ifstream c_shader_file;
    c_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        c_shader_file.open(compute_path);
        std::stringstream cShaderStream;
        cShaderStream << c_shader_file.rdbuf();
        c_shader_file.close();
        compute_code = cShaderStream.str();
    } catch (std::ifstream::failure& e) {
        throw std::runtime_error("ERROR::COMPUTE_SHADER::FILE_NOT_READ: " + std::string(compute_path));
    }

    const char* cShaderCode = compute_code.c_str();
    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    CheckCompileErrors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");
    glDeleteShader(compute);
}

ComputeShader::~ComputeShader() {glDeleteProgram(ID); }

void ComputeShader::Bind() const { glUseProgram(ID); }

void ComputeShader::Dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const {
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

void ComputeShader::Wait() const { glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); } 
void ComputeShader::SetFloat(const std::string &name, float value) const { glUniform1f(glGetUniformLocation(ID, name.c_str()), value); }
void ComputeShader::SetInt(const std::string &name, int value) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), value); }
void ComputeShader::SetVec3(const std::string &name, const glm::vec3 &vec) const { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &vec[0]); }


void ComputeShader::CheckCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            throw std::runtime_error("ERROR::COMPUTE_SHADER_COMPILATION:\n" + std::string(infoLog));
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            throw std::runtime_error("ERROR::COMPUTE_PROGRAM_LINKING:\n" + std::string(infoLog));
        }
    }
}