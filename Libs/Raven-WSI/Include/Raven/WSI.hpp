#pragma once

#include <Raven/WSI_Export.h>

#include <cstdint>
#include <set>
#include <string>

namespace Raven {
struct Keyboard {
  uint32_t UnicodeChar;
  uint32_t RepeatedKey;
  std::set<uint32_t> PressedKeys;
  std::set<uint32_t> ReleasedKeys;
  std::set<uint32_t> HeldKeys;
};

struct Mouse {
  struct Vec2 {
    double x;
    double y;
  };

  bool InWindow;
  bool WasInWindow;
  Vec2 Position;
  Vec2 LastPosition;
  Vec2 Scroll;
  std::set<uint32_t> PressedButtons;
  std::set<uint32_t> ReleasedButtons;
  std::set<uint32_t> HeldButtons;
};

RAVEN_WSI_EXPORT bool CreateWindow(unsigned int width,
                                   unsigned int height,
                                   const std::string& title);
RAVEN_WSI_EXPORT void DestroyWindow();

RAVEN_WSI_EXPORT bool WindowShouldClose();
RAVEN_WSI_EXPORT void WindowEventProcessing();

RAVEN_WSI_EXPORT bool WindowResized();

RAVEN_WSI_EXPORT const Keyboard& GetKeyboard();
RAVEN_WSI_EXPORT const Mouse& GetMouse();
}  // namespace Raven