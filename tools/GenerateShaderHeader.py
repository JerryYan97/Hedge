# Compile HLSL prebuilt/builtin shader to spirv.
# Put spirv into a header file that can be used in the engine.
# Currently, the script only supports vertex shader and fragment shader.
import os
import subprocess


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


def CompileShader(ShaderPathName, folderName, shaderType):
    shaderFlag = "vs_6_0"
    if shaderType == "Frag":
        shaderFlag = "ps_6_0"
    result = subprocess.run(["dxc.exe", "-spirv", "-T", shaderFlag, "-E", "main", ShaderPathName, "-Fo", ShaderPathName + ".spv"])


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
        if "Vert" in fileName and "hlsl" in fileName:
            CompileShader(Path + "\\" + fileName, FolderName, "Vert")
        elif "Frag" in fileName and "hlsl" in fileName:
            CompileShader(Path + "\\" + fileName, FolderName, "Frag")


if __name__ == "__main__":
    shadersPath = os.getcwd() + "\\..\\shaders"
    generator = os.walk(shadersPath)

    folders = next(generator)
    shaderFoldersPathsNameList = []
    for folderName in folders[1]:
        DeleteSpirvInFolder(shadersPath + "\\" + folderName)
        CompileShadersInFolder(shadersPath + "\\" + folderName, folderName)
        shaderFolderName = shadersPath + "\\" + folderName
        shaderFoldersPathsNameList.append(shaderFolderName)
    GenerateHeader(shaderFoldersPathsNameList, shadersPath)
