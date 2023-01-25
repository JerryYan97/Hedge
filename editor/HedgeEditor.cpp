#include "HedgeEditor.h"
#include "util/Utils.h"
#include "render/HRenderManager.h"
#include "logging/HLogger.h"
#include <iostream>
#include <cstdlib>

Hedge::HFrameListener* g_pFrameListener = new Hedge::HedgeEditor();

namespace Hedge
{
    // ================================================================================================================
    HedgeEditor::HedgeEditor()
    {
    }

    // ================================================================================================================
    HedgeEditor::~HedgeEditor()
    {
    }

    // ================================================================================================================
    void HedgeEditor::Run()
    {
        std::cout << "Hello World From the Editor" << std::endl;
    }

    // ================================================================================================================
    void HedgeEditor::FrameStarted()
    {}

    // ================================================================================================================
    void HedgeEditor::FrameEnded()
    {}

    // ================================================================================================================
    void HedgeEditor::BuildGame(
        const char* pPathFileName)
    {
        std::system("cmake -BC:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -S C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/ -G Ninja");
        std::system("ninja -C C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -j 6");
    }
}
