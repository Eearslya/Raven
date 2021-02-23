#include "Mouse.hpp"

#include <GLFW/glfw3.h>

namespace Raven {
void MousePreProcessing(Mouse& mouse) {
  mouse.WasInWindow = mouse.InWindow;
  mouse.LastPosition = mouse.Position;
  mouse.Scroll = {0, 0};
  mouse.PressedButtons.clear();
  mouse.ReleasedButtons.clear();
}

void OnMouseMoved(Mouse& mouse, double x, double y) { mouse.Position = {x, y}; }

void OnMouseEnter(Mouse& mouse, int entered) { mouse.InWindow = entered; }

void OnMouseScroll(Mouse& mouse, double x, double y) { mouse.Scroll = {x, y}; }

void OnMouseButton(Mouse& mouse, int button, int action, int) {
  if (action == GLFW_PRESS) {
    mouse.PressedButtons.insert(button);
    mouse.HeldButtons.insert(button);
  } else if (action == GLFW_RELEASE) {
    mouse.ReleasedButtons.insert(button);
    mouse.HeldButtons.erase(button);
  }
}
}  // namespace Raven