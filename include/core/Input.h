#pragma once
#include <SDL2/SDL.h>
#include <SDL_scancode.h>

class Input {
public:
  static void Update();

  static bool IsKeyHeld(SDL_Scancode key);
  static bool IsMouseButtonHeld(int button);

  static float GetMouseDeltaX();
  static float GetMouseDeltaY();

private:
  static int s_deltaX;
  static int s_deltaY;
  static const Uint8 *s_keyboardState;
  static Uint32 s_mouseState;
};
