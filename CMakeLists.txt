cmake_minimum_required(VERSION 3.18)

project("Raven")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()

find_package(Vulkan REQUIRED)

add_subdirectory(External)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Build/Bin")
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
add_subdirectory(Source)

add_subdirectory(Assets)
add_subdirectory(Shaders)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "Raven")