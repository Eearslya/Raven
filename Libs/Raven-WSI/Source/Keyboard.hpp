#pragma once

#include <Raven/WSI.hpp>

namespace Raven {
void KeyboardPreProcessing(Keyboard& keyboard);
void OnKey(Keyboard& keyboard, int key, int, int action, int);
void OnChar(Keyboard& keyboard, unsigned int keycode);
}