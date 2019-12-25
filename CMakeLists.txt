cmake_minimum_required(VERSION 3.1)
project(ra VERSION 0.0.0.1)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

find_package("OpenGL" REQUIRED)
find_package("GLFW" REQUIRED)
find_package("GLM" REQUIRED)
find_package("assimp" REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${CMAKE_SOURCE_DIR}/include")

add_executable(
    ra
    "${CMAKE_SOURCE_DIR}/src/glad/glad.c"
    "${CMAKE_SOURCE_DIR}/src/stb/stb_image.c"
    "${CMAKE_SOURCE_DIR}/src/main.cxx"
)

target_include_directories(
    ra PUBLIC
    "${OPENGL_INCLUDE_DIR}"
    "${GLFW_INCLUDE_DIR}"
    "${GLM_INCLUDE_DIRS}"
    "${assimp_INCLUDE_DIRS}"
)

target_link_libraries(
    ra
    "${OPENGL_LIBRARIES}"
    "${GLFW_LIBRARIES}"
    "${assimp_LIBRARIES}"
)
