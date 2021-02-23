# Raven
Raven is a pet-project of mine, an attempt to create a functioning game engine with a focus on the rendering system written using Vulkan.

## Building
This project is built using CMake. Currently I am building using the Clang compiler on Windows. (Note: Not Clang-CL, just Clang.)
Other compilers may work, but I have not tested them.

**Caveat:** GLFW's CMake file currently has a bug with using Clang on Windows, as it assumes you are using Clang-CL and sets compiler flags incorrectly. In order to generate the Makefile correctly, for now, a manual change must be made to GLFW's `src/CMakeLists.txt` file.
```diff
-if (CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
-  # Tell Clang-CL that this is a Clang flag
-  target_compile_options(glfw PRIVATE "/clang:-Wall")
-else()
   target_compile_options(glfw PRIVATE "-Wall")
-endif()
```

## Inspiration / Credit

This project is a tangled web of different ideas, inspired by many tutorials and other similar projects out there, infused with my own
personal tastes and ideas.
Some notable examples that I would like to give credit to include:

* [FoE Engine](https://git.stabletec.com/foe/engine)
* [VkGuide](https://vkguide.dev)
* [Vulkan-Tutorial](https://vulkan-tutorial.com)