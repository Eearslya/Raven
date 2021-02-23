#include "Keyboard.hpp"

#include <GLFW/glfw3.h>

namespace Raven {
void KeyboardPreProcessing(Keyboard& keyboard) {
  keyboard.UnicodeChar = 0;
  keyboard.RepeatedKey = 0;
  keyboard.PressedKeys.clear();
  keyboard.ReleasedKeys.clear();
}

void OnKey(Keyboard& keyboard, int key, int, int action, int) {
  if (action == GLFW_PRESS) {
    keyboard.PressedKeys.insert(key);
    keyboard.HeldKeys.insert(key);
  } else if (action == GLFW_REPEAT) {
    keyboard.RepeatedKey = key;
  } else if (action == GLFW_RELEASE) {
    keyboard.ReleasedKeys.insert(key);
    keyboard.HeldKeys.erase(key);
  }
}

void OnChar(Keyboard& keyboard, unsigned int keycode) { keyboard.UnicodeChar = keycode; }
}  // namespace Raven