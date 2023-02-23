# Prebuilt shaders and how to generate the header including them in the engine

## Prebuilt shaders

Currently, shaders are written in GLSL for simplicity. I'll maybe try to write all shader also in HLSL for reference.

### BasicRenderer

## Generation tool and process

The `GenerateShaderHeader.py` under the tool folder is used to generating the prebuilt `g_prebuiltShaders.h` under the `shader` folder. The engine would include the prebuilt shader header and use the spirv shader in it for building internal pipelines.

```
glslc.exe -o ShaderPathName.spv -fshader-stage="shaderType" "ShaderPathName"
```

```
dxc.exe -spirv -T vs_6_0 -E main .\triangle.vert -Fo .\triangle.vert.spv
```

## Reference

1. [Diffuse shading model](https://learnopengl.com/Lighting/Basic-Lighting)
2. [HLSL in Vulkan](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/hlsl.adoc)
3. [HLSL to SPIR-V Feature Mapping Manual](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst)