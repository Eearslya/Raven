cmake_minimum_required(VERSION 3.13)
project(Raven CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GenerateExportHeader)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

add_subdirectory(External)
include(External/CMakePackages.cmake)

add_compile_definitions(GLFW_INCLUDE_VULKAN)

add_subdirectory(Libs)

add_executable(Raven)

add_subdirectory(Raven)
target_link_libraries(Raven PRIVATE Raven-Graphics Raven-Lib Raven-WSI)