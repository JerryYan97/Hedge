#include "HedgeEditor.h"
#include "util/Utils.h"
#include "render/HRenderManager.h"
#include "logging/HLogger.h"
#include "scene/HScene.h"
#include <iostream>
#include <cstdlib>

Hedge::HFrameListener* g_pFrameListener = new Hedge::HedgeEditor();

namespace Hedge
{
    // ================================================================================================================
    HedgeEditor::HedgeEditor()
        : m_pLayout(nullptr)
    {
        m_pScenes.push_back(new HScene());
        m_activeScene = 0;
    }

    // ================================================================================================================
    HedgeEditor::~HedgeEditor()
    {
        for (auto pScene : m_pScenes)
        {
            delete pScene;
        }
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

    // ================================================================================================================
    const HScene& HedgeEditor::GetActiveScene()
    {
        return *m_pScenes[m_activeScene];
    }
}
