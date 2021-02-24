#include <Raven/Graphics/Context.hpp>
#include <Raven/Log.hpp>
#include <Raven/WSI.hpp>

int main(int, char**) {
  Raven::CreateWindow(1600, 900, "Raven");
  Raven::Graphics::GraphicsContext ctx("Raven", 1);
  Log(General, Info, "Main application loop started.");
  while (!Raven::WindowGetShouldClose()) {
    Raven::WindowEventProcessing();
  }
  Log(General, Info, "Main application loop ended.");
  return 0;
}