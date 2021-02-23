cmake_minimum_required(VERSION 3.13)
project(Raven-Lib CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Raven-Lib INTERFACE)

target_include_directories(Raven-Lib INTERFACE Include)