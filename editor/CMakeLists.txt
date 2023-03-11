cmake_minimum_required(VERSION 3.8)
set(EDITOR_NAME "HedgeEditor")
project(HedgeEditor VERSION 0.1 LANGUAGES CXX)

set(EDITOR_SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditorGuiManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditorGuiManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditorRenderManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HedgeEditorRenderManager.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EDITOR_SRC_LIST})

add_subdirectory(../engine/ ./build/engine)

add_executable(${EDITOR_NAME} ${EDITOR_SRC_LIST})
target_compile_features(${EDITOR_NAME} PRIVATE cxx_std_17)

target_link_libraries(${EDITOR_NAME} LINK_PUBLIC HedgeEngine)
target_include_directories(${EDITOR_NAME} PUBLIC ../engine/
                                                 ../external/ImGUICustomLayout/)