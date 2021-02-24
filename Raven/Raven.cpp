#include <Raven/Graphics/Context.hpp>
#include <Raven/WSI.hpp>

int main(int, char**) {
  Raven::CreateWindow(1600, 900, "Raven");
  Raven::Graphics::GraphicsContext ctx("Raven", 1);
  while (!Raven::WindowGetShouldClose()) {
    Raven::WindowEventProcessing();
  }
  return 0;
}