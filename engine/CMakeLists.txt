cmake_minimum_required(VERSION 3.13)
project(HedgeEngine VERSION 0.1 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeUtils/CMakeUtil.cmake)

CheckVulkanSDK()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/glfw 
                 ${CMAKE_CURRENT_SOURCE_DIR}/build/glfw)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/YamlCpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/build/yaml)

set_target_properties(format PROPERTIES FOLDER "YamlCpp")
set_target_properties(yaml-cpp PROPERTIES FOLDER "YamlCpp")
set_target_properties(yaml-cpp-parse PROPERTIES FOLDER "YamlCpp")
set_target_properties(yaml-cpp-read PROPERTIES FOLDER "YamlCpp")
set_target_properties(yaml-cpp-sandbox PROPERTIES FOLDER "YamlCpp")

add_library(HedgeEngine STATIC)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/core)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/logging)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/render)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/scene)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/util)

set(DearImGUIPath ${CMAKE_CURRENT_SOURCE_DIR}/../external/DearImGUI)

target_sources(HedgeEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/entry.cpp
                                   ${DearImGUIPath}/imgui.cpp
                                   ${DearImGUIPath}/imgui_draw.cpp
                                   ${DearImGUIPath}/imgui_demo.cpp
                                   ${DearImGUIPath}/imgui_tables.cpp
                                   ${DearImGUIPath}/imgui_widgets.cpp
                                   ${DearImGUIPath}/backends/imgui_impl_vulkan.cpp
                                   ${DearImGUIPath}/backends/imgui_impl_glfw.cpp)

get_target_property(ENG_SRC_LIST HedgeEngine SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../ FILES ${ENG_SRC_LIST})

target_include_directories(HedgeEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/util/
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/spdLog/include
                                              $ENV{VULKAN_SDK}/Include
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/glfw/include
                                              ${DearImGUIPath}
                                              ${DearImGUIPath}/backends
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/VMA/include
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/EnTT/single_include/entt
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../shaders
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../builtInModels
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/tinyObj)

target_link_directories(HedgeEngine PUBLIC $ENV{VULKAN_SDK}/lib
                                           ${CMAKE_CURRENT_SOURCE_DIR}/build/glfw/src/Debug)

target_compile_features(HedgeEngine PRIVATE cxx_std_17)

target_link_libraries(HedgeEngine LINK_PUBLIC vulkan-1
                                              glfw3
                                              yaml-cpp)