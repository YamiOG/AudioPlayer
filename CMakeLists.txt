add_subdirectory(extern)

CMAKE_MINIMUM_REQUIRED(VERSION 3.7)

include_directories(extern/SDL2/include extern/SDL2_ttf extern/MiniAudio extern/stb/include)

project(AudioPlayer)

file(GLOB EXTERN_SOURCES "${CMAKE_SOURCE_DIR}/extern/stb/include/*.h" "${CMAKE_SOURCE_DIR}/extern/stb/src/*.cpp" "${CMAKE_SOURCE_DIR}/extern/Miniaudio/include/*.h" "${CMAKE_SOURCE_DIR}/extern/Miniaudio/src/*.c")

add_executable(AudioPlayer ${EXTERN_SOURCES} main.cpp)

target_link_libraries(AudioPlayer PUBLIC SDL2 SDL2main PUBLIC SDL2_ttf PUBLIC ghc_filesystem)