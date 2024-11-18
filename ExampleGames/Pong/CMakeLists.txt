cmake_minimum_required(VERSION 3.8)
set(GAME_NAME "Pong")
project(Pong VERSION 0.1 LANGUAGES CXX)

# add_executable(${GAME_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Pong.cpp)
add_executable(${GAME_NAME})

add_subdirectory(../../engine ./build/engine)
add_subdirectory(./code)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(${GAME_NAME} PRIVATE cxx_std_17)

target_link_libraries(${GAME_NAME} LINK_PUBLIC HedgeEngine)
target_include_directories(${GAME_NAME} PUBLIC ../../engine/)

# TODO: Need to consider the release build.
set(GAME_BINARY_DIR ${CMAKE_BINARY_DIR})

# Compile Shader, clean and copy the resources to the executable directory when the game builds.
# Deal with the building warning.
set(DUMMY_FILE dummy)
set_property(SOURCE DUMMY_FILE
             PROPERTY SYMBOLIC True)

add_custom_command(
        OUTPUT
            DUMMY_FILE
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/PackProject ${GAME_BINARY_DIR}/Debug
)

add_custom_target(PACK_PROJECT
                  DEPENDS DUMMY_FILE)

add_dependencies(${GAME_NAME} PACK_PROJECT)