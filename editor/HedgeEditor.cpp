#include "HedgeEditor.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>

Hedge::GlobalVariablesRAIIManager raiiManager;

Hedge::HFrameListener* g_pFrameListener = raiiManager.GetHedgeEditor();
Hedge::HRenderManager* g_pRenderManager = raiiManager.GetHedgeEditorRenderManager();
Hedge::HGpuRsrcManager* g_pGpuRsrcManager = raiiManager.GetGpuRsrcManager();

namespace Hedge
{
    // ================================================================================================================
    HedgeEditor::HedgeEditor()
        : HFrameListener()
    {
        m_pScenes.push_back(new HScene());
        m_activeScene = 0;

        m_pScenes[0]->SpawnEntity(new HCubeEntity(), GetEventManager());
        m_pScenes[0]->SpawnEntity(new HCameraEntity(), GetEventManager());
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
    void HedgeEditor::AppStarts()
    {
    }

    // ================================================================================================================
    void HedgeEditor::BuildGame(
        const char* pPathFileName)
    {
        std::system("cmake -BC:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -S C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/ -G Ninja");
        std::system("ninja -C C:/JiaruiYan/Projects/VulkanProjects/TestGameProject/build -j 6");
    }

    // ================================================================================================================
    void HedgeEditor::CreateGameProject(
        const std::string& rootDir, 
        const std::string& projName)
    {
        m_projName = projName;

        // Save the project configuration:
        m_projFilePath = rootDir + "\\" + projName + ".yml";
        

        // Save the scene configuration:
        std::string scenePathName = rootDir + "\\scene\\testScene.yml";
        GetSerializer().SerializeScene(scenePathName, *m_pScenes[m_activeScene]);
    }

    // ================================================================================================================
    HScene& HedgeEditor::GetActiveScene()
    {
        return *m_pScenes[m_activeScene];
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::GlobalVariablesRAIIManager()
    {
        m_pHedgeEditor              = new HedgeEditor();
        m_pHedgeEditorGuiManager    = new HedgeEditorGuiManager();
        m_pGpuRsrcManager           = new HGpuRsrcManager();
        m_pHedgeEditorRenderManager = new HedgeEditorRenderManager(m_pHedgeEditorGuiManager, m_pGpuRsrcManager);
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::~GlobalVariablesRAIIManager()
    {
        delete m_pHedgeEditorGuiManager;
        delete m_pHedgeEditorRenderManager;
        delete m_pGpuRsrcManager;
        delete m_pHedgeEditor;     
    }
}
