cmake_minimum_required(VERSION 3.8)
set(ENG_NAME "HedgeEngine")
project(HedgeEngine VERSION 0.1 LANGUAGES CXX)

set(ENG_SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/entry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/Utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/HRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/HRenderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/HLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/HLogger.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENG_SRC_LIST})

add_library(${ENG_NAME} STATIC ${ENG_SRC_LIST})

target_include_directories(${ENG_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/util/
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/spdLog/include)