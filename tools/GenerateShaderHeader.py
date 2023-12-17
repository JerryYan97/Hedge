# Compile HLSL prebuilt/builtin shader to spirv.
# Put spirv into a header file that can be used in the engine.
# Currently, the script only supports vertex shader and fragment shader.
import os
import subprocess
import sys


def GenerateShaderFormatedArray(hexStr, arrayName):
    shaderArrayStr = "    constexpr uint8_t " + arrayName + "[] = {\n"
    for idx in range(len(hexStr) // 2):
        eleStr = ""
        if idx % 16 == 0:
            eleStr += "       "
        eleStr += " 0x" + hexStr[2*idx : 2*idx + 2] + ","
        if idx % 16 == 15:
            eleStr += "\n"
        shaderArrayStr += eleStr
    shaderArrayStr = shaderArrayStr[:-1]
    shaderArrayStr += "};\n"
    return shaderArrayStr


def GeneratePreShaderArrayStr():
    preShadersStr = "// ATTENTION: This file is generated from HLSL shaders and the GenerateShaderHeader.py. Don't edit it manually!\n"
    preShadersStr += "#pragma once\n\n"
    preShadersStr += "namespace Hedge\n"
    preShadersStr += "{\n"
    return preShadersStr


def GenerateHeader(shaderFoldersPathsNameList, shadersPath):
    generateHeaderHandle = open(shadersPath + "\\" + "g_prebuiltShaders.h", "w")

    generateHeaderHandle.write(GeneratePreShaderArrayStr())

    for shaderFolderPathName in shaderFoldersPathsNameList:
        fileGenerator = os.walk(shaderFolderPathName)
        filenames = next(fileGenerator)
        for fileName in filenames[2]:
            if ".spv" in fileName:
                with open(shaderFolderPathName + "\\" + fileName, mode='rb') as file: # b is important -> binary
                    fileContent = file.read()
                    hexStr = fileContent.hex()
                    arrayStr = GenerateShaderFormatedArray(hexStr, fileName.rsplit(".")[0] + "Script")
                    generateHeaderHandle.write(arrayStr)
                    generateHeaderHandle.write("\n")
    
    generateHeaderHandle.write("}")
    generateHeaderHandle.close()


def SelectDxc():
    pathEnvStr = os.environ['PATH']
    pathEnvStrList = pathEnvStr.rsplit(';')
    foundVulkanSDK = False
    dxcCmdStr = ''
    for path in pathEnvStrList:
        if 'VulkanSDK' in path:
            foundVulkanSDK = True
            dxcCmdStr = path

    if foundVulkanSDK is False:
        sys.exit('Cannot find the Vulkan SDK in the PATH environment variable.')

    return dxcCmdStr + '\\dxc.exe'


def CompileShaderHlsl(shaderPathName, folderPath, shaderType):
    shaderFlag = ""
    if shaderType == "vert":
        shaderFlag = "vs_6_1"
    elif shaderType == "frag":
        shaderFlag = "ps_6_1"
    else:
        sys.exit('Unrecogonized hlsl shader type.')
    
    subprocess.check_output([
        "C:\\VulkanSDK\\1.3.236.0\\Bin\\dxc.exe",
        '-spirv',
        '-T', shaderFlag,
        '-E', 'main',
        '-I', folderPath + "\\..\\shared",
        '-fspv-target-env=vulkan1.3',
        '-fspv-extension=SPV_KHR_ray_query',
        '-fspv-extension=SPV_KHR_ray_tracing',
        '-fspv-extension=SPV_KHR_multiview',
        '-fspv-extension=SPV_KHR_shader_draw_parameters',
        '-fspv-extension=SPV_EXT_descriptor_indexing',
        shaderPathName,
        '-Fo', shaderPathName + ".spv"
    ])


def CompileShaderGlsl(ShaderPathName, shaderType):
    result = subprocess.run(["glslc.exe", "-o", ShaderPathName + ".spv", "-fshader-stage=" + shaderType, ShaderPathName])


def DeleteSpirvInFolder(Path):
    fileGenerator = os.walk(Path)
    filenames = next(fileGenerator)
    for fileName in filenames[2]:
        if ".spv" in fileName:
            os.remove(Path + "\\" + fileName)


def CompileShadersInFolder(Path, FolderName):
    fileGenerator = os.walk(Path)
    filenames = next(fileGenerator)
    for fileName in filenames[2]:
        if "Vert" in fileName and "glsl" in fileName:
            CompileShaderGlsl(Path + "\\" + fileName, "vert")
        elif "Frag" in fileName and "glsl" in fileName:
            CompileShaderGlsl(Path + "\\" + fileName, "frag")
        elif "vert" in fileName and "hlsl" in fileName:
            CompileShaderHlsl(Path + "\\" + fileName, Path, "vert")
        elif "frag" in fileName and "hlsl" in fileName:
            CompileShaderHlsl(Path + "\\" + fileName, Path, "frag")


if __name__ == "__main__":
    file_path = os.path.realpath(__file__)
    idx = max(file_path.rfind('/'), file_path.rfind('\\'))
    folder_path = file_path[:idx]
    shadersPath = folder_path + "\\..\\shaders"
    generator = os.walk(shadersPath)

    folders = next(generator)
    shaderFoldersPathsNameList = []
    for folderName in folders[1]:
        DeleteSpirvInFolder(shadersPath + "\\" + folderName)
        CompileShadersInFolder(shadersPath + "\\" + folderName, folderName)
        shaderFolderName = shadersPath + "\\" + folderName
        shaderFoldersPathsNameList.append(shaderFolderName)
    GenerateHeader(shaderFoldersPathsNameList, shadersPath)
