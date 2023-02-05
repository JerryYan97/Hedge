#include "HedgeEditor.h"
#include "util/Utils.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include "imgui.h"
#include <iostream>
#include <cstdlib>

Hedge::HFrameListener* g_pFrameListener = new Hedge::HedgeEditor();
Hedge::HRenderManager* g_pRenderManager = new Hedge::HedgeEditorRenderManager(new Hedge::HedgeEditorGuiManager());

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
        
        pGuiManager->GenerateImGuiData(GetCurrentRenderImgView());
    }

    // ================================================================================================================
    HedgeEditorGuiManager::HedgeEditorGuiManager()
    {}

    // ================================================================================================================
    HedgeEditorGuiManager::~HedgeEditorGuiManager()
    {}

    // ================================================================================================================
    void HedgeEditorGuiManager::GenerateImGuiData(VkImageView* pResultImgView)
    {
        static bool use_work_area = true;
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        bool open = true;

        // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
        // Based on your use case you may want one of the other.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
        ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

        if (ImGui::Begin("Example: Fullscreen window", &open, flags))
        {
            ImGui::Checkbox("Use work area instead of main area", &use_work_area);
            ImGui::SameLine();

            ImGui::CheckboxFlags("ImGuiWindowFlags_NoBackground", &flags, ImGuiWindowFlags_NoBackground);
            ImGui::CheckboxFlags("ImGuiWindowFlags_NoDecoration", &flags, ImGuiWindowFlags_NoDecoration);
            ImGui::Indent();
            ImGui::CheckboxFlags("ImGuiWindowFlags_NoTitleBar", &flags, ImGuiWindowFlags_NoTitleBar);
            ImGui::CheckboxFlags("ImGuiWindowFlags_NoCollapse", &flags, ImGuiWindowFlags_NoCollapse);
            ImGui::CheckboxFlags("ImGuiWindowFlags_NoScrollbar", &flags, ImGuiWindowFlags_NoScrollbar);
            ImGui::Unindent();

            if (&open && ImGui::Button("Close this window"))
                open = false;
        }
        ImGui::End();
    }
}
