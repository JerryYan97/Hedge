#include "PongGame.h"
#include "../scene/HScene.h"
#include "../core/HAssetRsrcManager.h"
#include "yaml-cpp/yaml.h"
#include "Utils.h"
#include "UtilMath.h"
#include "imgui.h"
#include <map>

Hedge::GlobalVariablesRAIIManager g_raiiManager;

Hedge::HFrameListener* g_pFrameListener = g_raiiManager.GetGame();
Hedge::HRenderManager* g_pRenderManager = g_raiiManager.GetGameRenderManager();
Hedge::HGpuRsrcManager* g_pGpuRsrcManager = g_raiiManager.GetGpuRsrcManager();
Hedge::HAssetRsrcManager* g_pAssetRsrcManager = g_raiiManager.GetAssetRsrcManager();

namespace Hedge
{
    // ================================================================================================================
    HPongGame::HPongGame()
        : HFrameListener(),
          m_pScene(nullptr)
    {}

    // ================================================================================================================
    HPongGame::~HPongGame()
    {
        if (m_pScene != nullptr)
        {
            delete m_pScene;
        }
    }

    // ================================================================================================================
    void HPongGame::AppStarts()
    {
        m_pScene = new HScene();

        std::string exePathName = GetExePath();
        std::string exePath = GetFileDir(exePathName);

        // Read in the game settings
        YAML::Node config = YAML::LoadFile(exePath + "/gameConfig.yml");
        m_gameName = config["Game Name"].as<std::string>();
        g_raiiManager.GetGameRenderManager()->SetWindowTitle(m_gameName);

        std::string firstSceneName = config["First Scene"].as<std::string>();

        // Read in the first scene
        std::string firstSceneNamePath = exePath + "/scene/" + firstSceneName;
        GetSerializer().DeserializeYamlToScene(firstSceneNamePath, *m_pScene, GetEventManager());
    }

    // ================================================================================================================
    HGameGuiManager::HGameGuiManager()
        : HBaseGuiManager()
    {}

    // ================================================================================================================
    HGameGuiManager::~HGameGuiManager()
    {}

    // ================================================================================================================
    VkExtent2D HGameGuiManager::GetRenderExtent()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        uint32_t newWidth = std::max(static_cast<uint32_t>(viewport->WorkSize.x),
            static_cast<uint32_t>(64));
        uint32_t newHeight = std::max(static_cast<uint32_t>(viewport->WorkSize.y),
            static_cast<uint32_t>(64));

        return VkExtent2D{ newWidth, newHeight };
    }

    // ================================================================================================================
    void HGameGuiManager::SendIOEvents(
        HScene& scene,
        HEventManager& eventManager)
    {
        // Middle mouse event generation and passing
        {
            HEventArguments args;
            bool isDown = ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonMiddle);
            args[crc32("IS_DOWN")] = isDown;

            if (isDown)
            {
                HFVec2 pos;
                ImVec2 imPos = ImGui::GetMousePos();
                pos.ele[0] = imPos[0];
                pos.ele[1] = imPos[1];
                args[crc32("POS")] = pos;
            }

            HEvent mEvent(args, "MOUSE_MIDDLE_BUTTON");
            eventManager.SendEvent(mEvent, &scene);
        }

        // Key WASD event generation and passing
        {
            HEventArguments args;
            args[crc32("IS_DOWN")] = ImGui::IsKeyDown(ImGuiKey_W);

            HEvent mEvent(args, "KEY_W");
            eventManager.SendEvent(mEvent, &scene);
        }
        {
            HEventArguments args;
            args[crc32("IS_DOWN")] = ImGui::IsKeyDown(ImGuiKey_S);

            HEvent mEvent(args, "KEY_S");
            eventManager.SendEvent(mEvent, &scene);
        }
        {
            HEventArguments args;
            args[crc32("IS_DOWN")] = ImGui::IsKeyDown(ImGuiKey_A);

            HEvent mEvent(args, "KEY_A");
            eventManager.SendEvent(mEvent, &scene);
        }
        {
            HEventArguments args;
            args[crc32("IS_DOWN")] = ImGui::IsKeyDown(ImGuiKey_D);

            HEvent mEvent(args, "KEY_D");
            eventManager.SendEvent(mEvent, &scene);
        }
    }

    // ================================================================================================================
    void HGameGuiManager::GenerateImGuiData(
        VkImageView* resultImgView,
        VkExtent2D resultImgExtent,
        uint32_t frameIdx)
    {
        // Create a large image and window covers the whole screen
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        VkDescriptorSet my_image_texture = 0;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        if (ImGui::Begin("Game Render Window", nullptr, flags))
        {
            AddTextureToImGUI(&my_image_texture, resultImgView, frameIdx);
            ImGui::Image((ImTextureID)my_image_texture,
                         ImVec2(static_cast<float>(resultImgExtent.width),
                                static_cast<float>(resultImgExtent.height)));
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    // ================================================================================================================
    HGameRenderManager::HGameRenderManager(
        HBaseGuiManager* pGuiManager,
        HGpuRsrcManager* pGpuRsrcManager)
        : HRenderManager(pGuiManager, pGpuRsrcManager)
    {}

    // ================================================================================================================
    HGameRenderManager::~HGameRenderManager()
    {}

    // ================================================================================================================
    void HGameRenderManager::DrawHud(
        HFrameListener* pFrameListener)
    {
        HGameGuiManager* pGuiManager = dynamic_cast<HGameGuiManager*>(m_pGuiManager);

        pGuiManager->GenerateImGuiData(GetCurrentRenderImgView(), GetCurrentRenderImgExtent(), GetCurSwapchainFrameIdx());
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::GlobalVariablesRAIIManager()
    {
        CreateCustomGlobalVariables();
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::~GlobalVariablesRAIIManager()
    {
        delete m_pGameGuiManager;
        delete m_pGameRenderManager;

        delete m_pGameTemplate;
        delete m_pAssetRsrcManager;
        delete m_pGpuRsrcManager;
    }

    // ================================================================================================================
    void GlobalVariablesRAIIManager::CreateCustomGlobalVariables()
    {
        m_pGameTemplate = new HPongGame();
        m_pGameGuiManager = new HGameGuiManager();
        m_pGpuRsrcManager = new HGpuRsrcManager();
        m_pGameRenderManager = new HGameRenderManager(m_pGameGuiManager, m_pGpuRsrcManager);
        m_pAssetRsrcManager = new HAssetRsrcManager();

        std::string exePathName = GetExePath();
        std::string exePath = GetFileDir(exePathName);
        m_pAssetRsrcManager->UpdateAssetFolderPath(exePath);
    }
}