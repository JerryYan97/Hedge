#include "HedgeEditor.h"
#include "util/Utils.h"
#include "UtilMath.h"
#include "render/HRenderManager.h"
#include "scene/HScene.h"
#include "core/HEntity.h"
#include "imgui.h"
#include "CustomDearImGuiLayout.h"
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
        HBaseGuiManager* pGuiManager,
        HGpuRsrcManager* pGpuRsrcManager)
        : HRenderManager(pGuiManager, pGpuRsrcManager)
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
        
        pGuiManager->GenerateImGuiData(GetCurrentRenderImgView(), GetCurrentRenderImgExtent(), GetCurSwapchainFrameIdx());
    }

    // ================================================================================================================
    HedgeEditorGuiManager::HedgeEditorGuiManager()
        : m_pLayout(CreateGuiLayout())
    {}

    // ================================================================================================================
    HedgeEditorGuiManager::~HedgeEditorGuiManager()
    {
        delete m_pLayout;
    }

    // ================================================================================================================
    VkExtent2D HedgeEditorGuiManager::GetRenderExtent()
    {
        m_pLayout->ResizeAll();

        uint32_t newWidth = std::max(static_cast<uint32_t>(m_pRenderWindowNode->GetDomainSize().x), 
                                     static_cast<uint32_t>(64));
        uint32_t newHeight = std::max(static_cast<uint32_t>(m_pRenderWindowNode->GetDomainSize().y), 
                                      static_cast<uint32_t>(64));

        return VkExtent2D{ newWidth, newHeight };
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::GenerateImGuiData(
        VkImageView* pResultImgView,
        VkExtent2D resultImgExtent,
        uint32_t     frameIdx)
    {
        m_frameIdx = frameIdx;
        m_pRenderResultImgView = pResultImgView;
        m_renderResultImgExtent = resultImgExtent;
        m_pLayout->BeginEndLayout();
    }

    // ================================================================================================================
    DearImGuiExt::CustomLayout* HedgeEditorGuiManager::CreateGuiLayout()
    {
        DearImGuiExt::CustomLayoutNode* pRoot = new DearImGuiExt::CustomLayoutNode(0.8f);

        // Left and right splitter
        pRoot->CreateLeftChild(0.8f);
        pRoot->CreateRightChild(0.3f);

        // Left splitter's top and bottom windows
        DearImGuiExt::CustomLayoutNode* pLeftDomain = pRoot->GetLeftChild();
        pLeftDomain->CreateLeftChild(SceneRenderWindow);
        pLeftDomain->CreateRightChild(AssetWindow);
        m_pRenderWindowNode = pLeftDomain->GetLeftChild();

        // Right splitter's top and bottom windows
        DearImGuiExt::CustomLayoutNode* pRightDomain = pRoot->GetRightChild();
        pRightDomain->CreateLeftChild(SceneObjectsListWindow);
        pRightDomain->CreateRightChild(ObjectPropertiesWindow);

        return new DearImGuiExt::CustomLayout(pRoot);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::SceneRenderWindow()
    {
        HedgeEditorGuiManager* pGui = raiiManager.GetHedgeEditorGuiManager();

        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                                        ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        VkDescriptorSet my_image_texture = 0;

        if (ImGui::Begin("Scene Render Window", nullptr, flags))
        {
            pGui->AddTextureToImGUI(&my_image_texture, pGui->m_pRenderResultImgView, pGui->m_frameIdx);
            ImGui::Image((ImTextureID)my_image_texture, 
                         ImVec2(static_cast<float>(pGui->m_renderResultImgExtent.width), 
                                static_cast<float>(pGui->m_renderResultImgExtent.height)));
        }
        ImGui::End();
        ImGui::PopStyleVar(1);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::AssetWindow()
    {
        constexpr ImGuiWindowFlags TestWindowFlag = ImGuiWindowFlags_NoSavedSettings | 
                                                    ImGuiWindowFlags_NoCollapse | 
                                                    ImGuiWindowFlags_NoResize | 
                                                    ImGuiWindowFlags_NoMove | 
                                                    ImGuiWindowFlags_NoDecoration;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::Begin("AssetWindow", nullptr, TestWindowFlag);

        if (ImGui::TreeNode("Basic trees"))
        {
            for (int i = 0; i < 5; i++)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "Child %d", i))
                {
                    ImGui::Text("blah blah");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("button")) {}
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        ImGui::End();
        ImGui::PopStyleVar(1);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::SceneObjectsListWindow()
    {
        constexpr ImGuiWindowFlags TestWindowFlag = ImGuiWindowFlags_NoSavedSettings | 
                                                    ImGuiWindowFlags_NoCollapse | 
                                                    ImGuiWindowFlags_NoResize | 
                                                    ImGuiWindowFlags_NoMove | 
                                                    ImGuiWindowFlags_NoDecoration;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::Begin("SceneObjectsListWindow", nullptr, TestWindowFlag);

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Basic trees"))
        {
            for (int i = 0; i < 5; i++)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "Child %d", i))
                {
                    ImGui::Text("blah blah");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("button")) {}
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        ImGui::End();
        ImGui::PopStyleVar(1);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::ObjectPropertiesWindow()
    {
        constexpr ImGuiWindowFlags TestWindowFlag = ImGuiWindowFlags_NoSavedSettings | 
                                                    ImGuiWindowFlags_NoCollapse | 
                                                    ImGuiWindowFlags_NoResize | 
                                                    ImGuiWindowFlags_NoMove | 
                                                    ImGuiWindowFlags_NoDecoration;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::Begin("ObjectPropertiesWindow", nullptr, TestWindowFlag);

        if (ImGui::TreeNode("Basic trees"))
        {
            for (int i = 0; i < 5; i++)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "Child %d", i))
                {
                    ImGui::Text("blah blah");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("button")) {}
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        ImGui::End();
        ImGui::PopStyleVar(1);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::SendIOEvents(
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
