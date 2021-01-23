if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

set(GLSL_SOURCE_FILES)

#file(TOUCH ${GLSL_SOURCE_FILES})

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SHADER_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders")

file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

foreach(FILE ${GLSL_SOURCE_FILES})
  message(STATUS "Building Shader")
  get_filename_component(FILE_NAME ${FILE} NAME)
  set(GLSL "${SHADER_SRC_DIR}/${FILE}")
  set(SPIRV "${SHADER_BIN_DIR}/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(FILE)

add_custom_target(Shaders SOURCES ${GLSL_SOURCE_FILES} DEPENDS ${SPIRV_BINARY_FILES})