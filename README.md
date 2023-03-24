# Hedge engine and editor

Currently, the hedge engine and editor is a personal game engine or rendering framework for small projects.

![Cover Image](./imgs/Cover.PNG)

## Prerequisite

This project relies on [VulkanSDK]((https://vulkan.lunarg.com/sdk/home)), [CMake](https://cmake.org/), [Ninja](https://ninja-build.org/manual.html), [Python](https://www.python.org/), [WindowSDK and MSVC](https://visualstudio.microsoft.com/vs/community/). Please have them ready before you continue on. One recommendation configuration is to install Visual studio for WindowsSDK and MSVC and intall others separately.

## Clone this repo

```
git clone --recurse-submodules https://github.com/JerryYan97/Hedge.git
```

## How to build the editor

Execute the following cmake command under the `editor` folder.

```
xxxx\Hedge\editor>cmake -Bbuild -G "Visual Studio 16 2019" -S .
```

Please note that the edtior visual studio solution contains the engine project and you shouldn't build the engine separately for the editor.

## How to build the engine

```
xxxx\Hedge\engine>cmake -Bbuild -G "Visual Studio 16 2019" -S .
```

The engine solution would build three static libraries: `glfw3.lib`, `HedgeEngine.lib` and `yaml-cpp.lib`. If you want to output them into one folder, you can set the `HEDGE_LIB` environment variable to the folder gathering these static libraries. In addition, you also need to gather all header files in the engine source code. This can be done by using the `tools/GatherHeaderFiles.py` and executing the following command:

```
xxxx\Hedge\tools> python GatherHeaderFiles.py .....
```

## How to create a game project and build a game

If you are only interested in using the editor to make a game, you don't have to build the engine by yourself. There are prebuilt editor, necessary static libraries and headers released with this project.

The only configuration that you need is to fill the `HEDGE_LIB` environment variable with the directory containing these released files.

## For more details

Please refer to documents under the [./doc/DDN](https://github.com/JerryYan97/Hedge/tree/main/doc/DDN).

## Third parties dependencies

* [Vulkan](https://vulkan.lunarg.com/sdk/home)
* [VMA](https://gpuopen.com/vulkan-memory-allocator/)
* [EnTT](https://github.com/skypjack/entt)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [Yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [ImGUICustomLayout](https://github.com/JerryYan97/ImGUIExplore)
* [spdLog](https://github.com/gabime/spdlog)
* [Glfw](https://www.glfw.org/)