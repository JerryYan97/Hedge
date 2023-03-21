#include "HedgeEditor.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include "Utils.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <fstream>

#include <filesystem>

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
        // Generate cmake file
        std::string cmakeFileFolder = m_rootDir + "\\build";
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
        SaveGameConfig();

        GenCMakeFile(true);

        // Delete the game solution if it exists
        std::string cmakeFileFolder = m_rootDir + "\\build";
        std::filesystem::remove_all((cmakeFileFolder + "/build"));

        // Build game solution
        std::string cmakeBuildSolCmd;
        cmakeBuildSolCmd += "cmake -B";
        cmakeBuildSolCmd += (cmakeFileFolder + "/build");
        cmakeBuildSolCmd += " -S ";
        cmakeBuildSolCmd += cmakeFileFolder;
        cmakeBuildSolCmd += " -G \"Visual Studio 16 2019\"";
        std::system(cmakeBuildSolCmd.c_str());
    }

    // ================================================================================================================
    void HedgeEditor::BuildAndReleaseGame(
        const std::string& tarDir)
    {
        SaveGameConfig();

        GenCMakeFile(false);

        // Delete the game solution if it exists
        std::string cmakeFileFolder = m_rootDir + "\\build";
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
        const auto copyOptions = std::filesystem::copy_options::directories_only;
        if (std::filesystem::exists(tarDir + "\\scene"))
        {
            std::filesystem::remove_all(tarDir + "\\scene");
        }
        std::filesystem::create_directory(tarDir + "\\scene");
        std::filesystem::copy(m_rootDir + "\\scene", tarDir + "\\scene");
    }

    // ================================================================================================================
    void HedgeEditor::CreateGameProject(
        const std::string& rootDir, 
        const std::string& projName)
    {
        m_projName = projName;
        m_projFilePath = rootDir + "\\" + projName + ".yml";
        raiiManager.GetHedgeEditorRenderManager()->SetWindowTitle(m_projName);

        // Save the scene configuration:
        std::filesystem::create_directory(rootDir + "\\scene");
        std::string scenePathName = rootDir + "\\scene\\testScene.yml";
        GetSerializer().SerializeScene(scenePathName, *m_pScenes[m_activeScene]);

        // Save the project configuration:
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

        // First scene
        ymlProjEmitter << YAML::Key << "First Scene";
        ymlProjEmitter << YAML::Value << "testScene.yml";

        ymlProjEmitter << YAML::EndMap;
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

        m_projFilePath = pathName;
        m_rootDir = Hedge::GetFileDir(pathName);

        // Deseriazlie the target project
        YAML::Node config = YAML::LoadFile(pathName.c_str());
        m_projName = config["Project Name"].as<std::string>();
        m_gameName = config["Game Name"].as<std::string>();
        raiiManager.GetHedgeEditorRenderManager()->SetWindowTitle(m_projName);

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
