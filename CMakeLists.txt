cmake_minimum_required(VERSION 3.16)
project(AudioPlayer C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(VENDORS_DIR "${CMAKE_SOURCE_DIR}/vendors")

file(GLOB AUDIOPLAYER_SOURCES "${SRC_DIR}/*.cpp")

add_library(miniaudio STATIC "${VENDORS_DIR}/miniaudio/miniaudio.c")
target_include_directories(miniaudio PRIVATE "${VENDORS_DIR}/miniaudio")

add_library(tinyfiledialogs STATIC "${VENDORS_DIR}/tinyfiledialogs/tinyfiledialogs.c")
target_include_directories(tinyfiledialogs PRIVATE "${VENDORS_DIR}/tinyfiledialogs")

add_executable(AudioPlayer "${AUDIOPLAYER_SOURCES}")

target_include_directories(AudioPlayer PRIVATE "${INCLUDE_DIR}" "${VENDORS_DIR}/miniaudio" "${VENDORS_DIR}/tinyfiledialogs")

target_link_libraries(AudioPlayer PRIVATE miniaudio tinyfiledialogs)