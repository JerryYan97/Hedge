# Prebuilt shaders and how to generate the header including them in the engine

This design doc describes the design of the Hedge game engine project organization. This includes all relevant static libraries, dynamic libraries, relocatbale objects, executable objects and the generation of executable objects like the editor or a game.

## Prebuilt shaders

* Engine: The Hedge game engine itself is a static library that contains most code of the engine. This library is used by linking it into the game editor or the game application executable. It contains the entry point of the game executable but it doesn't contain the editor's entry point.

* Editor: The Hedge game engine editor is an executable. It has its own code that is separate from the engine's static library and needs to be linked to the engine library to build itself. The editor should be able to create a game project and package the game project into a game executable with assets. The game developer should be able to configure the engine editor by building a dynamic library that can be linked to the editor during editor's runtime. The editor has its own entry point that is different from the game application.

* Game project: A game project contains all the assets and custom code that a game should have. The custom code should be able to be built in a dynamic linking library that can be used to customize the game editor. And, these custom code will be compiled to relocatable objects and linked to the engine static libaray when the editor builds the final game executable. 

* Game: A final game should be an executable with all relevant data. It is built from the game projects' custom code and the engine static library.

## Generation tool and process

```
dxc.exe -spirv -T vs_6_0 -E main .\triangle.vert -Fo .\triangle.vert.spv
```

## Reference

1. [Diffuse shading model](https://learnopengl.com/Lighting/Basic-Lighting)
2. [HLSL in Vulkan](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/hlsl.adoc)
3. [HLSL to SPIR-V Feature Mapping Manual](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst)