#pragma once

#include <Raven/WSI_Export.h>

#include <cstdint>
#include <set>
#include <string>
#include <tuple>
#include <vector>

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

RAVEN_WSI_EXPORT std::string WindowGetTitle();
RAVEN_WSI_EXPORT void WindowSetTitle(const std::string& title);
RAVEN_WSI_EXPORT bool WindowGetShouldClose();
RAVEN_WSI_EXPORT void WindowSetShouldClose(bool close);
RAVEN_WSI_EXPORT std::tuple<unsigned int, unsigned int> WindowGetSize();
RAVEN_WSI_EXPORT void WindowSetSize(unsigned int width, unsigned int height);

RAVEN_WSI_EXPORT void WindowEventProcessing();

RAVEN_WSI_EXPORT bool WindowResized();

RAVEN_WSI_EXPORT const Keyboard& GetKeyboard();
RAVEN_WSI_EXPORT const Mouse& GetMouse();
RAVEN_WSI_EXPORT const std::vector<const char*> WindowGetVulkanExtensions();
}  // namespace Raven