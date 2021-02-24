cmake_minimum_required(VERSION 3.13)
project(Raven-Graphics CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Raven-Graphics)

generate_export_header(Raven-Graphics EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/Exports/Raven/Graphics_Export.h)
target_include_directories(Raven-Graphics PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Exports)
target_link_libraries(Raven-Graphics PUBLIC Raven-Lib Raven-WSI Vulkan::Vulkan)

add_subdirectory(Source)