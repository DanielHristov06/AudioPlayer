cmake_minimum_required(VERSION 3.16)
project(AudioPlayer C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(VENDORS_DIR "${CMAKE_SOURCE_DIR}/vendors")

file(GLOB AUDIOPLAYER_SOURCES "${SRC_DIR}/*.cpp" "${VENDORS_DIR}/glad/src/glad.c")

add_library(miniaudio STATIC "${VENDORS_DIR}/miniaudio/miniaudio.c")
target_include_directories(miniaudio PRIVATE "${VENDORS_DIR}/miniaudio")

add_library(tinyfiledialogs STATIC "${VENDORS_DIR}/tinyfiledialogs/tinyfiledialogs.c")
target_include_directories(tinyfiledialogs PRIVATE "${VENDORS_DIR}/tinyfiledialogs")

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory("${VENDORS_DIR}/glfw")

add_executable(AudioPlayer "${AUDIOPLAYER_SOURCES}")

target_include_directories(AudioPlayer PRIVATE
    "${INCLUDE_DIR}"
    "${VENDORS_DIR}/miniaudio"
    "${VENDORS_DIR}/tinyfiledialogs"
    "${VENDORS_DIR}/glfw/include"
    "${VENDORS_DIR}/glad/include"
    "${VENDORS_DIR}/imgui"
    "${VENDORS_DIR}/imgui/backends"
)

target_sources(AudioPlayer PRIVATE
    "${VENDORS_DIR}/imgui/imgui.cpp"
    "${VENDORS_DIR}/imgui/imgui_draw.cpp"
    "${VENDORS_DIR}/imgui/imgui_tables.cpp"
    "${VENDORS_DIR}/imgui/imgui_widgets.cpp"
    "${VENDORS_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${VENDORS_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
    "${VENDORS_DIR}/imgui/imgui_demo.cpp"
)

target_compile_definitions(AudioPlayer PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD GLFW_INCLUDE_NONE)

target_link_libraries(AudioPlayer PRIVATE miniaudio tinyfiledialogs)

if (WIN32)
    target_link_libraries(AudioPlayer PRIVATE glfw opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(AudioPlayer PRIVATE glfw OpenGL::GL)
endif()