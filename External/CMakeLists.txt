find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/Eearslya/imgui.git
	GIT_TAG docking)
FetchContent_MakeAvailable(imgui)
set_property(TARGET imgui PROPERTY FOLDER "Dependencies")

set(sdl2_DIR "SDL_PATH" CACHE FILEPATH "Path to SDL2")
add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE "${sdl2_DIR}/include" "${sdl2_image_DIR}/include" )
target_link_directories(sdl2 INTERFACE "${sdl2_DIR}/lib/x64" "${sdl2_image_DIR}/lib/x64")
target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

FetchContent_Declare(
	fmt
	GIT_REPOSITORY git@github.com:fmtlib/fmt.git
	GIT_TAG 7.1.3)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY FOLDER "Dependencies")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")