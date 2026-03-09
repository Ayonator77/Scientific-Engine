#include "core/Camera.h"
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>

Camera::Camera(int width, int height)
    : m_target(0.0f, 0.0f, 0.0f), m_radius(4.0f), m_pitch(0.0f), m_yaw(0.0f),
      m_screen_width(width), m_screen_height(height) {}

glm::mat4 Camera::GetViewMatrix() const {
  // Convert spherical coordinates (radius, pitch, yaw) to Cartesiann (x, y, z)
  float x = m_radius * std::cos(glm::radians(m_pitch)) *
            std::sin(glm::radians(m_yaw));
  float y = m_radius * std::sin(glm::radians(m_pitch));
  float z = m_radius * std::cos(glm::radians(m_pitch)) *
            std::cos(glm::radians(m_yaw));

  glm::vec3 position = m_target + glm::vec3(x, y, z);

  return glm::lookAt(position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix() const {
  return glm::perspective(glm::radians(45.0f),
                          (float)m_screen_width / (float)m_screen_height, 0.1f,
                          100.0f);
}

void Camera::ProcessMouseMovement(float x_offset, float y_offset) {
  const float sensitivity = 0.3f;
  m_yaw += x_offset * sensitivity;
  m_pitch += y_offset * sensitivity;

  // constrain pitch to prevent the camera from flipping upside down
  if (m_pitch > 89.0f)
    m_pitch = 89.0f;
  if (m_pitch < -89.0f)
    m_pitch = -89.0f;
}

void Camera::ProcessMouseScroll(float y_offset) {
  const float zoom_speed = 0.5f;
  m_radius -= y_offset * zoom_speed;

  // clamp the zoom so we dont phase through the planet
  if (m_radius < 1.2f)
    m_radius = 1.2f;
  if (m_radius > 20.0f)
    m_radius = 20.0f;
}
