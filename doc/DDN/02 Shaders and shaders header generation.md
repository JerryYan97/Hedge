# Prebuilt shaders and how to generate the header including them in the engine

There are prebuilt shaders in the hedge engine to give basic rendering functionalities. They are written in glsl under the `shaders` and compiled into spirv bytes into the `shaders/g_prebuiltShaders.h`. Here is a list of prebuilt shaders descriptions:

* BasicRenderer: A simple Blinn-Phong model following [1]. The glsl code is put under the `shaders/BasicRenderer` and the spirv bytes are stored in the `BasicRendererFragScript` and `BasicRendererVertScript`.

## Generation tool and process

The `tools/GenerateShaderHeader.py` under the tool folder is used to generate the prebuilt `g_prebuiltShaders.h` under the `shader` folder. The engine would include the prebuilt shader header and use the spirv shader in it for building internal pipelines. The python script essentially runs the glslc on the system, which should be installed when you install the vulkan sdk.

```
glslc.exe -o ShaderPathName.spv -fshader-stage="shaderType" "ShaderPathName"
```

You can also write shader in the HLSL and generate its spirv code by using the `dxc.exe` which is also included in the vulkan sdk.

```
dxc.exe -spirv -T vs_6_0 -E main .\triangle.vert -Fo .\triangle.vert.spv
```

## Reference

1. [Diffuse shading model](https://learnopengl.com/Lighting/Basic-Lighting)
2. [HLSL in Vulkan](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/hlsl.adoc)
3. [HLSL to SPIR-V Feature Mapping Manual](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst)