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
std::string gWindowTitle;
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

  gWindowTitle = title;
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

RAVEN_WSI_EXPORT std::string WindowGetTitle() { return gWindowTitle; }

RAVEN_WSI_EXPORT void WindowSetTitle(const std::string& title) {
  glfwSetWindowTitle(gWindow, title.c_str());
  gWindowTitle = title;
}

bool WindowGetShouldClose() { return glfwWindowShouldClose(gWindow); }

RAVEN_WSI_EXPORT void WindowSetShouldClose(bool close) {
  glfwSetWindowShouldClose(gWindow, close ? GLFW_TRUE : GLFW_FALSE);
}

RAVEN_WSI_EXPORT std::tuple<unsigned int, unsigned int> WindowGetSize() {
  int width, height;
  glfwGetWindowSize(gWindow, &width, &height);
  return std::make_tuple(static_cast<unsigned int>(width), static_cast<unsigned int>(height));
}

RAVEN_WSI_EXPORT void WindowSetSize(unsigned int width, unsigned int height) {
  glfwSetWindowSize(gWindow, width, height);
}

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