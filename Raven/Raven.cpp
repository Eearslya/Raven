#include <Raven/WSI.hpp>

int main(int, char**) {
  Raven::CreateWindow(1600, 900, "Raven");
  while (!Raven::WindowGetShouldClose()) {
    Raven::WindowEventProcessing();
  }
  return 0;
}