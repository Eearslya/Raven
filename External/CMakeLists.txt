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

FetchContent_Declare(
	fmt
	GIT_REPOSITORY git@github.com:fmtlib/fmt.git
	GIT_TAG 7.1.3)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY FOLDER "Dependencies")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tinygltf")