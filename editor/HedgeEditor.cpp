#include "HedgeEditor.h"
#include "util/Utils.h"
#include "render/HRenderManager.h"
#include "logging/HLogger.h"
#include <iostream>
#include <cstdlib>

HedgeEditor::HedgeEditor()
{
}

HedgeEditor::~HedgeEditor()
{
}

void HedgeEditor::Run()
{
    std::cout << "Hello World From the Editor" << std::endl;
}

void HedgeEditor::BuildGame(
    const char* pPathFileName)
{
    std::system("cmake -BC:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -S C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/ -G Ninja");
    std::system("ninja -C C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -j 6");
}

void main(
    int    argc,
    char** argv)
{
    HedgeEditor editor;
    editor.Run();
    UtilPrint();
    Hedge::HLogger logger;

    Hedge::HRenderManager renderManager;

    HDG_CORE_WARN("Init Log!");
    HDG_INFO("Another Log from client.");
}
