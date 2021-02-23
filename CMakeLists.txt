cmake_minimum_required(VERSION 3.13)
project(Raven CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(External)
add_subdirectory(Libs)

add_executable(Raven)

add_subdirectory(Raven)
target_link_libraries(Raven PRIVATE Raven-WSI)