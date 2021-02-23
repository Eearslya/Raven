cmake_minimum_required(VERSION 3.13)
project(Raven CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Raven-WSI)

add_subdirectory(Source)

target_include_directories(Raven-WSI PUBLIC Include)
target_link_libraries(Raven-WSI PUBLIC glfw)