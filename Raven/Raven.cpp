#include <Raven/WSI.hpp>

int main(int, char**) {
  Raven::CreateWindow(1600, 900, "Raven");
  while (!Raven::WindowShouldClose()) {
    Raven::WindowEventProcessing();
  }
  return 0;
}