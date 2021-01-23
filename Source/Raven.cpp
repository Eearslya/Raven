#include "Core.h"

#include "Application.h"

using namespace Raven;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  Log::Initialize();
  Log::SetLevel(Raven::Log::Level::Trace);

  try {
    const std::vector<const char*> cmdArgs(__argv, __argv + __argc);

    Application app(cmdArgs);
  } catch (const std::exception& e) {
    const std::string alert{
        fmt::format("An application exception has occurred.\n{}\n\n{}", typeid(e).name(), e.what())};
    Log::Fatal(alert);
    ::MessageBoxA(NULL, alert.c_str(), "Raven Exception", MB_OK | MB_ICONERROR);
  } catch (...) {
    Log::Fatal("An unknown application exception has occurred.");
    ::MessageBoxA(NULL, "An unknown application exception has occurred.", "Raven Exception",
                  MB_OK | MB_ICONERROR);
  }

  Log::Shutdown();
}