set(IMAGE_FILES)

set(MODEL_FILES
    Models/Suzanne.bin
    Models/Suzanne.gltf
    Models/Suzanne_BaseColor.png
    Models/Suzanne_MetallicRoughness.png)

set(ASSET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ASSET_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets")
set(IMAGE_BIN_DIR "${ASSET_BIN_DIR}/Images")
set(MODEL_BIN_DIR "${ASSET_BIN_DIR}/Models")

file(MAKE_DIRECTORY ${ASSET_BIN_DIR})
file(MAKE_DIRECTORY ${IMAGE_BIN_DIR})
file(MAKE_DIRECTORY ${MODEL_BIN_DIR})

add_custom_target(Assets)
#add_custom_command(TARGET Assets POST_BUILD
#    WORKING_DIRECTORY ${ASSET_SRC_DIR}
#    COMMAND ${CMAKE_COMMAND} -E copy ${IMAGE_FILES} ${IMAGE_BIN_DIR})
add_custom_command(TARGET Assets POST_BUILD
    WORKING_DIRECTORY ${ASSET_SRC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${MODEL_FILES} ${MODEL_BIN_DIR})