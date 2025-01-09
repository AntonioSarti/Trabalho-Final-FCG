add_executable(
    ${PROJECT_NAME}
    main.cpp
    collisions.cpp
    ambient.cpp
)

# Add a custom command to copy the data directory
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    ${CMAKE_BINARY_DIR}/data
)
