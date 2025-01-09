cmake_minimum_required(VERSION 3.5)

project(fcg)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable LSP to function correctly 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set directories for compiled artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set CMP0079 policy to NEW
if (POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

find_package(OpenGL REQUIRED)

add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(data)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE include
)

target_link_libraries(
    ${PROJECT_NAME}
    glfw
    glad
    glm
    stb_image
    tiny_obj_loader
    OpenGL::GL
)

add_dependencies(${PROJECT_NAME} copy_shaders)
