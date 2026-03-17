#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
  Camera(int width, int height);

  // input handlers
  void ProcessMouseMovement(float x_offset, float y_offset);
  void ProcessMouseScroll(float y_offset);

  // matrix generators
  glm::mat4 GetViewMatrix() const;
  glm::mat4 GetProjectionMatrix() const;
  void UpdateResolution(int width, int height);

private:
  glm::vec3 m_target; // center of the planet
  float m_radius;     // distance from the planet
  float m_pitch;      // up/down angle
  float m_yaw;        // left/right angle

  int m_screen_width;
  int m_screen_height;
};
