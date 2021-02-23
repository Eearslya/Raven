#pragma once

#include <Raven/WSI.hpp>

namespace Raven {
void MousePreProcessing(Mouse& mouse);
void OnMouseMoved(Mouse& mouse, double x, double y);
void OnMouseEnter(Mouse& mouse, int entered);
void OnMouseScroll(Mouse& mouse, double x, double y);
void OnMouseButton(Mouse& mouse, int button, int action, int);
}