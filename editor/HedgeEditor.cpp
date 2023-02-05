#include "HedgeEditor.h"
#include "util/Utils.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include "imgui.h"
#include <iostream>
#include <cstdlib>

Hedge::GlobalVariablesRAIIManager raiiManager;

Hedge::HFrameListener* g_pFrameListener = raiiManager.GetHedgeEditor();
Hedge::HRenderManager* g_pRenderManager = raiiManager.GetHedgeEditorRenderManager();

namespace Hedge
{
    // ================================================================================================================
    HedgeEditor::HedgeEditor()
        : m_pLayout(nullptr)
    {
        m_pScenes.push_back(new HScene());
        m_activeScene = 0;

        m_pScenes[0]->SpawnEntity(new HCubeEntity());
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
    HScene& HedgeEditor::GetActiveScene()
    {
        return *m_pScenes[m_activeScene];
    }

    // ================================================================================================================
    HedgeEditorRenderManager::HedgeEditorRenderManager(
        HBaseGuiManager* pGuiManager)
        : HRenderManager(pGuiManager)
    {}

    // ================================================================================================================
    HedgeEditorRenderManager::~HedgeEditorRenderManager()
    {
    }

    // ================================================================================================================
    void HedgeEditorRenderManager::DrawHud(
        HFrameListener* pFrameListener)
    {
        HedgeEditorGuiManager* pGuiManager = dynamic_cast<HedgeEditorGuiManager*>(m_pGuiManager);
        
        pGuiManager->GenerateImGuiData(GetCurrentRenderImgView(), m_activeRendererIdx);
    }

    // ================================================================================================================
    HedgeEditorGuiManager::HedgeEditorGuiManager()
    {}

    // ================================================================================================================
    HedgeEditorGuiManager::~HedgeEditorGuiManager()
    {}

    // ================================================================================================================
    void HedgeEditorGuiManager::GenerateImGuiData(
        VkImageView* pResultImgView,
        uint32_t frameIdx)
    {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
        // Based on your use case you may want one of the other.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        VkDescriptorSet my_image_texture = 0;

        if (ImGui::Begin("Example: Fullscreen window", nullptr, flags))
        {
            ImVec2 winContentExtentUL = ImGui::GetWindowContentRegionMax();
            ImVec2 winContentExtentDR = ImGui::GetWindowContentRegionMin();
            ImVec2 winContentExtent;
            winContentExtent.x = winContentExtentUL.x - winContentExtentDR.x;
            winContentExtent.y = winContentExtentUL.y - winContentExtentDR.y;

            AddTextureToImGUI(&my_image_texture, pResultImgView, frameIdx);
            ImGui::Image((ImTextureID)my_image_texture, winContentExtent);
        }
        ImGui::End();
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::GlobalVariablesRAIIManager()
    {
        m_pHedgeEditor              = new HedgeEditor();
        m_pHedgeEditorGuiManager    = new HedgeEditorGuiManager();
        m_pHedgeEditorRenderManager = new HedgeEditorRenderManager(m_pHedgeEditorGuiManager);
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::~GlobalVariablesRAIIManager()
    {
        delete m_pHedgeEditorGuiManager;
        delete m_pHedgeEditorRenderManager;
        delete m_pHedgeEditor;     
    }
}
