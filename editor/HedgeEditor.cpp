#include "HedgeEditor.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <fstream>

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
        std::ofstream projConfigFileHandle(m_projFilePath.c_str());
        YAML::Emitter ymlProjEmitter(projConfigFileHandle);

        // MISC project config
        ymlProjEmitter << YAML::BeginMap;

        // Engine version
        ymlProjEmitter << YAML::Key << "Engine Version Major";
        ymlProjEmitter << YAML::Value << HEDGE_ENGINE_MAJOR_VERSION;
        ymlProjEmitter << YAML::Key << "Engine Version Minor";
        ymlProjEmitter << YAML::Value << HEDGE_ENGINE_MINOR_VERSION;

        // Project name, packaged game name
        ymlProjEmitter << YAML::Key << "Project Name";
        ymlProjEmitter << YAML::Value << "Test Project";
        ymlProjEmitter << YAML::Key << "Game Name";
        ymlProjEmitter << YAML::Value << "TestGame";

        ymlProjEmitter << YAML::EndMap;

        // Save the scene configuration:
        std::string scenePathName = rootDir + "\\scene\\testScene.yml";
        GetSerializer().SerializeScene(scenePathName, *m_pScenes[m_activeScene]);
    }

    // ================================================================================================================
    void HedgeEditor::OpenGameProject(
        const std::string& pathName)
    {
        // Close the current project
        for (auto itr : m_pScenes)
        {
            delete itr;
        }
        m_pScenes.clear();

        // Deseriazlie the target project


        // Load in the active scene
        HScene* pTmpScene = new HScene();
        GetSerializer().DeserializeYamlToScene(pathName, *pTmpScene, GetEventManager());
        m_pScenes.push_back(pTmpScene);
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
