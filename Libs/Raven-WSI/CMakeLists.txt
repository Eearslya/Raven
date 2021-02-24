cmake_minimum_required(VERSION 3.13)
project(Raven-WSI CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Raven-WSI)

add_subdirectory(Source)

generate_export_header(Raven-WSI EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/Exports/Raven/WSI_Export.h)
target_include_directories(Raven-WSI PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Exports)
target_link_libraries(Raven-WSI PUBLIC glfw Vulkan::Vulkan)