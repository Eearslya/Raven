#include "Core.h"

#include <fmt/chrono.h>
#include <fmt/color.h>

#include "Log.h"
#include "Win32.h"

namespace Raven {
Log::Level Log::sLogLevel{Level::Info};
constexpr const char* gLevelTags[]{"FTL", "ERR", "WRN", "INF", "DBG", "TRC"};
const fmt::v7::text_style gLevelColors[]{
    fmt::v7::fg(fmt::color::black) | fmt::v7::bg(fmt::color::red),  // Fatal
    fmt::v7::fg(fmt::color::crimson),                               // Error
    fmt::v7::fg(fmt::color::dark_golden_rod),                       // Warn
    fmt::v7::fg(fmt::color::white),                                 // Info
    fmt::v7::fg(fmt::color::dark_cyan),                             // Debug
    fmt::v7::fg(fmt::color::gray)                                   // Trace
};

void Log::Initialize() noexcept {
  ::AllocConsole();
  ::AttachConsole(::GetCurrentProcessId());

  HWND consoleHandle{::GetConsoleWindow()};
  ::ShowWindow(consoleHandle, SW_SHOWMAXIMIZED);

  HANDLE outHandle{::GetStdHandle(STD_OUTPUT_HANDLE)};
  HANDLE errHandle{::GetStdHandle(STD_ERROR_HANDLE)};
  DWORD outMode, errMode;
  ::GetConsoleMode(outHandle, &outMode);
  ::GetConsoleMode(errHandle, &errMode);
  outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  errMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  ::SetConsoleMode(outHandle, outMode);
  ::SetConsoleMode(errHandle, errMode);

  FILE* stream;
  freopen_s(&stream, "CONOUT$", "w+", stdout);
  freopen_s(&stream, "CONOUT$", "w+", stderr);
  ::SetConsoleTitle(TEXT("Raven Console"));
}

void Log::Shutdown() noexcept { ::FreeConsole(); }

void Log::SetLevel(const Level level) noexcept { sLogLevel = level; }

void Log::Output(const Level level, const std::string& msg) noexcept {
  const std::chrono::time_point now{std::chrono::system_clock::now()};
  const std::string finalMsg{
      fmt::format("<{:%H:%M:%S}> [{}] {}\n", now, gLevelTags[static_cast<size_t>(level)], msg)};
  fmt::print(gLevelColors[static_cast<size_t>(level)], finalMsg);
  ::OutputDebugStringA(finalMsg.c_str());
}
}  // namespace Raven