cmake_minimum_required(VERSION 3.13)
project(HedgeEngine VERSION 0.1 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeUtils/CMakeUtil.cmake)

CheckVulkanSDK()

if(NOT DEFINED EDITOR_BUILD)
    # Build for game
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/glfw 
                     ${CMAKE_CURRENT_SOURCE_DIR}/build/glfw)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/YamlCpp
                     ${CMAKE_CURRENT_SOURCE_DIR}/build/yaml)
else()
    # Build for editor
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/glfw 
                     ${EDITOR_CMAKE_DIR}/build/build/engine/glfw)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/YamlCpp
                     ${EDITOR_CMAKE_DIR}/build/build/engine/yaml)
endif()

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

add_dependencies(HedgeEngine glfw)

if(NOT DEFINED EDITOR_BUILD)
    # Build for game
    # Check whether use game templates
    if(NOT DEFINED USE_GAME_TEMPLATE)
        set(USE_GAME_TEMPLATE ON)
    endif()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/game)
    if(DEFINED ENV{HEDGE_LIB})
        set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG $ENV{HEDGE_LIB})
        set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE $ENV{HEDGE_LIB})

        set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG $ENV{HEDGE_LIB})
        set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE $ENV{HEDGE_LIB})

        set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG $ENV{HEDGE_LIB})
        set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE $ENV{HEDGE_LIB})

        set(ENGINE_LIB_DIR $ENV{HEDGE_LIB})
    else()
        set(ENGINE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build)

        set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
        set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})

        set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
        set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})

        set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
        set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})
    endif()
else()
    # Build from the editor and for the editor only (Exclude game templates)
    set(ENGINE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build)

    set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
    set_target_properties(yaml-cpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})

    set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
    set_target_properties(glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})

    set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ENGINE_LIB_DIR})
    set_target_properties(HedgeEngine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ENGINE_LIB_DIR})
endif()

file(MAKE_DIRECTORY ${ENGINE_LIB_DIR})
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
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/tinyObj
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../external/tinygltf)

target_link_directories(HedgeEngine PUBLIC $ENV{VULKAN_SDK}/lib
                                           ${ENGINE_LIB_DIR})

target_compile_features(HedgeEngine PRIVATE cxx_std_17)

target_link_libraries(HedgeEngine LINK_PUBLIC vulkan-1
                                              glfw3
                                              yaml-cpp)

# Compile built in shaders
# Deal with the building warning.
set(DUMMY_FILE dummy)
set_property(SOURCE DUMMY_FILE
             PROPERTY SYMBOLIC True)

add_custom_command(
        OUTPUT
            DUMMY_FILE
        COMMAND python
            ${CMAKE_CURRENT_SOURCE_DIR}/../tools/GenerateShaderHeader.py
)

add_custom_target(SHADER_COMPILE
                  DEPENDS DUMMY_FILE)

add_dependencies(HedgeEngine SHADER_COMPILE)