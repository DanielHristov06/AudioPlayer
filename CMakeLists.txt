cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(AudioPlayer)

add_executable(AudioPlayer "src/main.cpp" "vendors/miniaudio/miniaudio.c" "src/AudioPlayer.cpp" "vendors/tinyfiledialogs/tinyfiledialogs.c")

target_include_directories(AudioPlayer PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/vendors/miniaudio" "${CMAKE_SOURCE_DIR}/vendors/tinyfiledialogs")