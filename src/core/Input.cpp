#include "core/Input.h"
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <sys/types.h>

int Input::s_deltaX = 0;
int Input::s_deltaY = 0;
const Uint8 *Input::s_keyboardState = nullptr;
Uint32 Input::s_mouseState = 0;

void Input::Update() {
  // Poll the os for hardware state
  s_keyboardState = SDL_GetKeyboardState(nullptr);
  s_mouseState = SDL_GetRelativeMouseState(&s_deltaX, &s_deltaY);
}

bool Input::IsKeyHeld(SDL_Scancode key) {
  if (!s_keyboardState)
    return false;
  return s_keyboardState[key];
}

bool Input::IsMouseButtonHeld(int button) {
  // SDL maps button 1 to bit 0, button 2 to bit 1, etc.
  return (s_mouseState & SDL_BUTTON(button)) != 0;
}

float Input::GetMouseDeltaX() { return static_cast<float>(s_deltaX); }
float Input::GetMouseDeltaY() { return static_cast<float>(s_deltaY); }
