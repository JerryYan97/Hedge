#include "HedgeEditor.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include "core/HAssetRsrcManager.h"
#include "Utils.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <fstream>

#include <filesystem>

Hedge::GlobalVariablesRAIIManager g_raiiManager;

Hedge::HFrameListener* g_pFrameListener = g_raiiManager.GetHedgeEditor();
Hedge::HRenderManager* g_pRenderManager = g_raiiManager.GetHedgeEditorRenderManager();
Hedge::HGpuRsrcManager* g_pGpuRsrcManager = g_raiiManager.GetGpuRsrcManager();
Hedge::HAssetRsrcManager* g_pAssetRsrcManager = g_raiiManager.GetAssetRsrcManager();
Hedge::HBaseGuiManager* g_pGuiManager = g_raiiManager.GetHedgeEditorGuiManager();

namespace Hedge
{
    // ================================================================================================================
    HedgeEditor::HedgeEditor()
        : HFrameListener(),
          m_activeScene(0)
    {
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
    void HedgeEditor::FrameStarted()
    {}

    // ================================================================================================================
    void HedgeEditor::FrameEnded()
    {}

    // ================================================================================================================
    void HedgeEditor::AppStarts()
    {
        LoadEditorRsrcs();

        g_raiiManager.GetHedgeEditorGuiManager()->AppStart();

        std::string defaultProjDir(getenv("HEDGE_LIB"));
        defaultProjDir += "\\DefaultProject\\Project.yml";
        OpenGameProject(defaultProjDir);        
    }

    // ================================================================================================================
    std::string HedgeEditor::GenGameCMakeFileStr(
        bool isDebug)
    {
        std::string cmakeStr;

        cmakeStr += "cmake_minimum_required(VERSION 3.8)\n";
        cmakeStr += "project(HedgeGame VERSION 0.1 LANGUAGES CXX)\n\n";

        cmakeStr += "add_executable(HedgeGame)\n\n";

        cmakeStr += "target_include_directories(HedgeGame PUBLIC $ENV{HEDGE_LIB}/headers)\n";
        cmakeStr += "target_link_directories(HedgeGame PUBLIC $ENV{HEDGE_LIB})\n";
        cmakeStr += "target_link_libraries(HedgeGame LINK_PUBLIC HedgeEngine\n";
        cmakeStr += "                                            vulkan-1\n";
        cmakeStr += "                                            glfw3\n";

        isDebug ? cmakeStr += "                                            yaml-cppd)\n" :
                  cmakeStr += "                                            yaml-cpp)\n";

        cmakeStr += "file(WRITE null.cpp "")\n\n";

        cmakeStr += "target_sources(HedgeGame PRIVATE null.cpp)\n";
        cmakeStr += "set_target_properties(HedgeGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/../)\n";
        cmakeStr += "set_target_properties(HedgeGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../)\n";

        return cmakeStr;
    }

    // ================================================================================================================
    void HedgeEditor::SaveGameConfig()
    {
        // Save game config file
        std::string gameConfigFileNamePath = m_rootDir + "\\gameConfig.yml";
        std::ofstream gameConfigFileHandle(gameConfigFileNamePath.c_str());
        YAML::Emitter ymlGameEmitter(gameConfigFileHandle);

        ymlGameEmitter << YAML::BeginMap;
        ymlGameEmitter << YAML::Key << "Game Name";
        ymlGameEmitter << YAML::Value << m_gameName;
        ymlGameEmitter << YAML::Key << "First Scene";
        ymlGameEmitter << YAML::Value << "testScene.yml";
        ymlGameEmitter << YAML::EndMap;
    }

    // ================================================================================================================
    void HedgeEditor::GenCMakeFile(
        bool isDebug)
    {
        std::string buildType;
        if (isDebug)
        {
            buildType = "Debug";
        }
        else
        {
            buildType = "Release";
        }

        // Generate cmake file
        std::string cmakeFileFolder = m_rootDir + "\\" + buildType + "Build";
        CreateDirectoryA(cmakeFileFolder.c_str(), NULL);

        std::string cmakeStr = GenGameCMakeFileStr(isDebug);

        std::string cmakeFilePathName = cmakeFileFolder + "\\CMakeLists.txt";
        std::ofstream gameCMakeFileHandle(cmakeFilePathName);
        gameCMakeFileHandle << cmakeStr;
        gameCMakeFileHandle.close();
    }

    // ================================================================================================================
    void HedgeEditor::BuildDebugGame()
    {
        if (m_rootDir.empty() == false)
        {
            // SaveGameConfig();

            GenCMakeFile(true);

            // Delete the game solution if it exists
            std::string cmakeFileFolder = m_rootDir + "\\DebugBuild";
            std::filesystem::remove_all((cmakeFileFolder + "/build"));

            // Build game solution
            std::string cmakeBuildSolCmd;
            cmakeBuildSolCmd += "cmake -B";
            cmakeBuildSolCmd += (cmakeFileFolder + "/build");
            cmakeBuildSolCmd += " -S ";
            cmakeBuildSolCmd += cmakeFileFolder;
            cmakeBuildSolCmd += " -G \"Visual Studio 17 2022\"";
            std::system(cmakeBuildSolCmd.c_str());
        }
    }

    // ================================================================================================================
    void HedgeEditor::BuildAndReleaseGame(
        const std::string& tarDir)
    {
        if (m_rootDir.empty() == false)
        {
            // SaveGameConfig();

            GenCMakeFile(false);

            // Delete the game solution if it exists
            std::string cmakeFileFolder = m_rootDir + "\\ReleaseBuild";
            std::filesystem::remove_all((cmakeFileFolder + "\\build"));

            // Build game in release mode by ninja
            std::string cmakeBuildNinjaCmd;
            cmakeBuildNinjaCmd += "cmake -B";
            cmakeBuildNinjaCmd += (cmakeFileFolder + "\\build");
            cmakeBuildNinjaCmd += " -S " + cmakeFileFolder;
            cmakeBuildNinjaCmd += " -G Ninja";
            cmakeBuildNinjaCmd += " -DCMAKE_BUILD_TYPE=Release";
            std::system(cmakeBuildNinjaCmd.c_str());

            std::string cmakeCompileCmd;
            cmakeCompileCmd += "ninja";
            cmakeCompileCmd += (" -C " + cmakeFileFolder + "\\build");
            cmakeCompileCmd += " -j 6";
            std::system(cmakeCompileCmd.c_str());

            // Copy game configuration file and executable file
            std::filesystem::remove_all(tarDir + "\\gameConfig.yml");
            std::filesystem::remove_all(tarDir + "\\HedgeGame.exe");
            std::filesystem::copy(m_rootDir + "\\gameConfig.yml", tarDir + "\\gameConfig.yml");
            std::filesystem::copy(m_rootDir + "\\HedgeGame.exe", tarDir + "\\HedgeGame.exe");

            // Copy resource folders
            // const auto copyOptions = std::filesystem::copy_options::directories_only;
            if (std::filesystem::exists(tarDir + "\\scene"))
            {
                std::filesystem::remove_all(tarDir + "\\scene");
            }
            std::filesystem::create_directory(tarDir + "\\scene");
            std::filesystem::copy(m_rootDir + "\\scene", tarDir + "\\scene");

            if (std::filesystem::exists(tarDir + "\\assets"));
            {
                std::filesystem::remove_all(tarDir + "assets");
            }
            std::filesystem::copy(m_rootDir + "\\assets",
                                  tarDir + "\\assets",
                                  std::filesystem::copy_options::recursive);
        }
    }

    // ================================================================================================================
    void HedgeEditor::GameProjectSaveAs(
        const std::string& rootDir)
    {
        // NOTE: Assume that we have already cleaned up the rsrc managed.
        const std::string folderName = GetNamePathFolderName(rootDir);

        // Copy and paste the current project.
        CopyFolder(m_rootDir, rootDir);

        OpenGameProject(rootDir + "\\Project.yml");
    }

    // ================================================================================================================
    void HedgeEditor::ReleaseCurrentProjectRsrc()
    {
        g_pGpuRsrcManager->WaitDeviceIdle();

        // Close the current project
        for (auto itr : m_pScenes)
        {
            delete itr;
        }
        m_pScenes.clear();

        HedgeEditorRenderManager* pEditorRenderManager = (HedgeEditorRenderManager*)g_pRenderManager;
        pEditorRenderManager->ReleaseAllInUseGpuRsrc();
        g_pAssetRsrcManager->ReleaseAllAssets();
        g_pGpuRsrcManager->CleanupAllRsrc();
    }

    // ================================================================================================================
    void HedgeEditor::OpenGameProject(
        const std::string& pathName)
    {
        m_projFilePath = pathName;
        m_rootDir = Hedge::GetFileDir(pathName);
        g_pAssetRsrcManager->UpdateAssetFolderPath(m_rootDir);

        // Deseriazlie the target project
        YAML::Node config = YAML::LoadFile(pathName.c_str());
        m_projName = config["Project Name"].as<std::string>();
        m_gameName = config["Game Name"].as<std::string>();
        g_raiiManager.GetHedgeEditorRenderManager()->SetWindowTitle(m_projName);

        // Load in the active scene aka first scene
        std::string firstSceneName = config["First Scene"].as<std::string>();
        HScene* pTmpScene = new HScene();
        GetSerializer().DeserializeYamlToScene(m_rootDir + "\\scene\\" + firstSceneName, *pTmpScene, GetEventManager());
        m_pScenes.push_back(pTmpScene);
    }

    // ================================================================================================================
    HScene& HedgeEditor::GetActiveScene()
    {
        return *m_pScenes[m_activeScene];
    }

    // ================================================================================================================
    void HedgeEditor::LoadEditorRsrcs()
    {
        
    }

    // ================================================================================================================
    SceneRenderInfo HedgeEditor::GetActiveSceneRenderInfo()
    {
        return m_pScenes[m_activeScene]->GetSceneRenderInfo();
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::GlobalVariablesRAIIManager()
    {
        m_pHedgeEditor              = new HedgeEditor();
        m_pHedgeEditorGuiManager    = new HedgeEditorGuiManager();
        m_pGpuRsrcManager           = new HGpuRsrcManager();
        m_pHedgeEditorRenderManager = new HedgeEditorRenderManager(m_pHedgeEditorGuiManager, m_pGpuRsrcManager);
        m_pAssetRsrcManager         = new HAssetRsrcManager();
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::~GlobalVariablesRAIIManager()
    {
        delete m_pHedgeEditorGuiManager;
        delete m_pHedgeEditorRenderManager;
        
        delete m_pHedgeEditor;
        delete m_pAssetRsrcManager;
        delete m_pGpuRsrcManager;
    }
}
