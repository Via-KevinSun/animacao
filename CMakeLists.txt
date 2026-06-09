cmake_minimum_required(VERSION 3.16)
project(Animacao)

set(CMAKE_CXX_STANDARD 17)

set(GLFW_DIR ${CMAKE_SOURCE_DIR}/libs/glfw)
set(GLEW_DIR ${CMAKE_SOURCE_DIR}/libs/glew)
set(GLM_DIR  ${CMAKE_SOURCE_DIR}/libs/glm)

include_directories(
    ${GLFW_DIR}/include
    ${GLEW_DIR}/include
    ${GLM_DIR}
)

link_directories(
    ${GLFW_DIR}/lib-mingw-w64
    ${GLEW_DIR}/lib/Release/x64
)

add_executable(animacao src/main.cpp)

target_compile_definitions(animacao PRIVATE GLEW_STATIC)

target_link_libraries(animacao
    glfw3
    glew32s
    opengl32
    gdi32
    user32
    kernel32
)

add_custom_command(TARGET animacao POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_DIR}/lib-mingw-w64/glfw3.dll"
        $<TARGET_FILE_DIR:animacao>
)