#include "core/Input.h"
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <sys/types.h>


void Input::Update() {
  // Poll the os for hardware state
  m_keyboardState = SDL_GetKeyboardState(nullptr);
  m_mouseState = SDL_GetRelativeMouseState(&m_deltaX, &m_deltaY);
}

bool Input::IsKeyHeld(SDL_Scancode key) const {
  if (!m_keyboardState) return false;
  return m_keyboardState[key];
}

bool Input::IsMouseButtonHeld(int button) const {
  return (m_mouseState & SDL_BUTTON(button)) != 0;
}

float Input::GetMouseDeltaX() const { return static_cast<float>(m_deltaX); }
float Input::GetMouseDeltaY() const { return static_cast<float>(m_deltaY); }