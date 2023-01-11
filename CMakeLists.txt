cmake_minimum_required(VERSION 3.8)
set(MY_APP_NAME "HedgeEditor")
project(HedgeEditor VERSION 0.1 LANGUAGES CXX)

include_directories("$ENV{VULKAN_SDK}/Include")
include_directories(./external/glfw/include)
include_directories(./external/DearImGUI)

set(DearImGUIPath ./external/DearImGUI)
include_directories(${DearImGUIPath}/backends)

include_directories(./external/ImGUICustomLayout/)
include_directories(./external/Gardener)
include_directories(./external/ImGUICustomLayout)
include_directories(${BOOST_ROOT})

link_directories("$ENV{VULKAN_SDK}/lib")
link_directories("./external/glfw/build/src/Debug/")

add_definitions(-DSOURCE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}\")

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.81.0 REQUIRED COMPONENTS
             fiber context)

set(EXT_SRC_LIST
    ${DearImGUIPath}/imgui.cpp
    ${DearImGUIPath}/imgui_draw.cpp
    ${DearImGUIPath}/imgui_demo.cpp
    ${DearImGUIPath}/imgui_tables.cpp
    ${DearImGUIPath}/imgui_widgets.cpp
    ${DearImGUIPath}/backends/imgui_impl_vulkan.cpp
    ${DearImGUIPath}/backends/imgui_impl_glfw.cpp
    ./external/Gardener/src/Gardener.cpp
    ./external/Gardener/src/GardenerUtils.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXT_SRC_LIST})

set(EDITOR_SRC_LIST
    ./src/HedgeEditor.cpp
    ./src/HedgeEditor.h
    ./src/render/HRenderer.h
    ./src/render/HRenderer.cpp
    ./src/util/UtilObjs.h
    ./src/util/UtilObjs.cpp
    ./src/util/Utils.h
    ./src/util/Utils.cpp
    ./src/main.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${EDITOR_SRC_LIST})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(${MY_APP_NAME} ${EDITOR_SRC_LIST}
                                  ${EXT_SRC_LIST})
    target_compile_features(${MY_APP_NAME} PRIVATE cxx_std_17)
    target_link_libraries(${MY_APP_NAME} vulkan-1
                                         glfw3
                                         Boost::fiber
                                         Boost::context)
else()
    message( FATAL_ERROR "Cannot find the Boost library." )
endif()
