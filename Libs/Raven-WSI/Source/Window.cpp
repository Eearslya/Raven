#include <GLFW/glfw3.h>

#include <Raven/WSI.hpp>
#include <mutex>

#include "Keyboard.hpp"
#include "Mouse.hpp"

namespace Raven {
namespace {
void WindowPreProcessing();
void WindowResizedCallback(GLFWwindow*, int, int);
void WindowKeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
void WindowCharCallback(GLFWwindow*, unsigned int keycode);
void WindowCursorPosCallback(GLFWwindow*, double x, double y);
void WindowCursorEnterCallback(GLFWwindow*, int entered);
void WindowCursorScrollCallback(GLFWwindow*, double x, double y);
void WindowCursorButtonCallback(GLFWwindow*, int button, int action, int mods);

std::once_flag gGlfwInitialized;
GLFWwindow* gWindow{nullptr};
bool gWindowResized{false};
Keyboard gKeyboard;
Mouse gMouse;

const auto glfwInitOnce{[]() { glfwInit(); }};

void WindowPreProcessing() { gWindowResized = false; }

void WindowResizedCallback(GLFWwindow*, int, int) { gWindowResized = true; }

void WindowKeyCallback(GLFWwindow*, int key, int scancode, int action, int mods) {
  OnKey(gKeyboard, key, scancode, action, mods);
}

void WindowCharCallback(GLFWwindow*, unsigned int keycode) { OnChar(gKeyboard, keycode); }

void WindowCursorPosCallback(GLFWwindow*, double x, double y) { OnMouseMoved(gMouse, x, y); }

void WindowCursorEnterCallback(GLFWwindow*, int entered) { OnMouseEnter(gMouse, entered); }

void WindowCursorScrollCallback(GLFWwindow*, double x, double y) { OnMouseScroll(gMouse, x, y); }

void WindowCursorButtonCallback(GLFWwindow*, int button, int action, int mods) {
  OnMouseButton(gMouse, button, action, mods);
}
}  // namespace

bool CreateWindow(unsigned int width, unsigned int height, const std::string& title) {
  if (gWindow != nullptr) {
    return false;
  }

  std::call_once(gGlfwInitialized, glfwInitOnce);

  gWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  glfwSetKeyCallback(gWindow, WindowKeyCallback);
  glfwSetCharCallback(gWindow, WindowCharCallback);
  glfwSetCursorPosCallback(gWindow, WindowCursorPosCallback);
  glfwSetCursorEnterCallback(gWindow, WindowCursorEnterCallback);
  glfwSetScrollCallback(gWindow, WindowCursorScrollCallback);
  glfwSetMouseButtonCallback(gWindow, WindowCursorButtonCallback);
  glfwSetWindowSizeCallback(gWindow, WindowResizedCallback);

  return true;
}

void DestroyWindow() {
  if (gWindow) {
    glfwDestroyWindow(gWindow);
  }
  gWindow = nullptr;
}

bool WindowShouldClose() { return glfwWindowShouldClose(gWindow); }

void WindowEventProcessing() {
  KeyboardPreProcessing(gKeyboard);
  MousePreProcessing(gMouse);
  WindowPreProcessing();
  glfwPollEvents();
}

bool WindowResized() { return gWindowResized; }

const Keyboard& GetKeyboard() { return gKeyboard; }

const Mouse& GetMouse() { return gMouse; }
}  // namespace Raven