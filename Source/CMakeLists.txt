set(ROOT_FILES
	Application.cpp
	Application.h
    Core.h
	Log.cpp
	Log.h
    Raven.cpp
	VulkanCore.h
	Win32.h
	Window.cpp
	Window.h)

set(VULKAN_FILES
	Vulkan/Instance.cpp
	Vulkan/Instance.h
	Vulkan/PhysicalDevice.cpp
	Vulkan/PhysicalDevice.h
	Vulkan/Surface.cpp
	Vulkan/Surface.h)
    
set(RAVEN_FILES
    ${ROOT_FILES}
	${VULKAN_FILES})

# For convenience, ensure any newly added source files
# get created if they don't already exist. Note that this
# will not create the directories automatically, only files.
file(TOUCH ${RAVEN_FILES})

add_executable(Raven ${RAVEN_FILES})

source_group("Vulkan" FILES ${VULKAN_FILES})
source_group("" FILES ${ROOT_FILES})

target_precompile_headers(Raven PRIVATE Core.h)
# Hide CMake's precompiled headers and source in their own source folder.
set(PCH_FILES
	${CMAKE_SOURCE_DIR}/Build/Source/CMakeFiles/Raven.dir/cmake_pch.cxx
	${CMAKE_SOURCE_DIR}/Build/Source/CMakeFiles/Raven.dir/Debug/cmake_pch.hxx
	${CMAKE_SOURCE_DIR}/Build/Source/CMakeFiles/Raven.dir/Release/cmake_pch.hxx
	${CMAKE_SOURCE_DIR}/Build/Source/CMakeFiles/Raven.dir/MinSizeRel/cmake_pch.hxx
	${CMAKE_SOURCE_DIR}/Build/Source/CMakeFiles/Raven.dir/RelWithDebInfo/cmake_pch.hxx)
source_group("Precompiled Headers" FILES ${PCH_FILES})

set_property(TARGET Raven PROPERTY CXX_STANDARD 17)
set_property(TARGET Raven PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Raven>")
target_include_directories(Raven PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Raven Vulkan::Vulkan glm imgui stb fmt)
add_dependencies(Raven Assets Shaders)

if (MSVC)
	target_link_options(Raven PRIVATE
		"/SUBSYSTEM:WINDOWS"       # Windows subsystem
	)
endif()