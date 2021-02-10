#include "Core.h"

#include "Vulkan/Instance.h"
#include "Vulkan/Surface.h"
#include "Window.h"

namespace Raven {
constexpr static const char* gWindowClassName{"Raven"};
static bool gWindowClassCreated{false};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CLOSE:
      ::PostQuitMessage(0);
      break;

    case WM_QUIT:
    case WM_DESTROY:
      return 0;
  }

  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

Window::Window() {
  const HINSTANCE inst{::GetModuleHandleA(nullptr)};

  if (!gWindowClassCreated) {
    const WNDCLASSEXA wndClass{
        sizeof(WNDCLASSEXA),  // cbSize
        CS_DBLCLKS,           // style,
        WndProc,              // lpfnWndProc
        0,                    // cbClsExtra
        0,                    // cbWndExtra
        inst,                 // hInstance,
        nullptr,              // hIcon
        nullptr,              // hCursor
        nullptr,              // hbrBackground
        nullptr,              // lpszMenuName
        gWindowClassName,     // lpszClassName
        nullptr               // hIconSm
    };
    ::RegisterClassExA(&wndClass);
    gWindowClassCreated = true;
  }

  const DWORD style{WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE};
  const DWORD exStyle{WS_EX_OVERLAPPEDWINDOW};

  RECT windowRect{};
  ::AdjustWindowRectEx(&windowRect, style, false, exStyle);

  const int screenW{::GetSystemMetrics(SM_CXSCREEN)};
  const int screenH{::GetSystemMetrics(SM_CYSCREEN)};
  const int windowW{1600 + (windowRect.right - windowRect.left)};
  const int windowH{900 + (windowRect.bottom - windowRect.top)};
  const int windowX{std::max((screenW - windowW) / 2, 0)};
  const int windowY{std::max((screenH - windowH) / 2, 0)};

  mHandle = ::CreateWindowExA(exStyle,           // dwExStyle
                              gWindowClassName,  // lpClassName
                              "Raven",           // lpWindowName
                              style,             // dwStyle
                              windowX,           // X
                              windowY,           // Y
                              windowW,           // nWidth
                              windowH,           // nHeight
                              nullptr,           // hWndParent
                              nullptr,           // hMenu
                              inst,              // hInstance
                              nullptr            // lpParam
  );
  ::ShowWindow(mHandle, SW_SHOW);
}

Window::~Window() { ::DestroyWindow(mHandle); }

bool Window::Update() noexcept {
  MSG msg;
  while (::PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      return false;
    }
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  return true;
}

void Window::SetTitle(const std::string& title) noexcept {
  ::SetWindowTextA(mHandle, title.c_str());
}

vk::UniqueSurfaceKHR Window::CreateSurface(const vk::Instance& instance) const {
  return instance.createWin32SurfaceKHRUnique({{}, ::GetModuleHandleA(nullptr), mHandle});
}
}  // namespace Raven