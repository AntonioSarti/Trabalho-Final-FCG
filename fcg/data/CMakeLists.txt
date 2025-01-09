# Define the list of shader files
set(SHADER_FILES
    ${CMAKE_SOURCE_DIR}/data/shaders/shader_vertex.glsl
    ${CMAKE_SOURCE_DIR}/data/shaders/shader_fragment.glsl
    # Add more shader files here
)

# Define the output directory for the shaders
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/data/shaders)

# Create the output directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Clear the list of shader output targets
set(SHADER_OUTPUTS)

# Add a custom command for each shader file to copy it to the build directory
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_FILE} ${SHADER_OUTPUT_DIR}/${SHADER_NAME}
        DEPENDS ${SHADER_FILE}
        COMMENT "Copying ${SHADER_FILE} to ${SHADER_OUTPUT_DIR}/${SHADER_NAME}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT_DIR}/${SHADER_NAME})
endforeach()

# Add a custom target to ensure shaders are copied before building
add_custom_target(copy_shaders ALL DEPENDS ${SHADER_OUTPUTS})
