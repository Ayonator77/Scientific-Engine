#pragma once
#include <glad/glad.h>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Direct all OpenGL draw commands to this hidden canvas
    void Bind() const;
    
    // Direct OpenGL draw commands back to the computer monitor
    void Unbind() const;
    
    // Handle window resizing
    void Resize(int width, int height);

    // Getters so our shaders can read the textures we drew
    unsigned int GetColorAttachment() const { return m_colorAttachment; }
    unsigned int GetDepthAttachment() const { return m_depthAttachment; }

private:
    void Invalidate();

    unsigned int m_FBO = 0;
    unsigned int m_colorAttachment = 0;
    unsigned int m_depthAttachment = 0;
    int m_width;
    int m_height;
};