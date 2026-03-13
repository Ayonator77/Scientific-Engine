#pragma once
#include <SDL2/SDL.h>
#include <SDL_scancode.h>

class Input {
public:
  Input() = default;

  void Update();

  bool IsKeyHeld(SDL_Scancode key) const;
  bool IsMouseButtonHeld(int button) const;

  float GetMouseDeltaX() const;
  float GetMouseDeltaY() const;

private:
  int m_deltaX = 0;
  int m_deltaY = 0;
  const Uint8 *m_keyboardState = nullptr;
  Uint32 m_mouseState = 0;
};