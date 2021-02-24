cmake_minimum_required(VERSION 3.13)
project(Raven-Lib CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Raven-Lib)

generate_export_header(Raven-Lib EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/Exports/Raven/Lib_Export.h)
target_include_directories(Raven-Lib PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Exports)
target_link_libraries(Raven-Lib PUBLIC fmt)

add_subdirectory(Source)