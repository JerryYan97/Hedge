#include "HedgeEditorGuiManager.h"

#include "imgui.h"
#include "CustomDearImGuiLayout.h"

#include "HedgeEditor.h"
#include "Utils.h"
#include "UtilMath.h"

extern Hedge::GlobalVariablesRAIIManager raiiManager;

namespace Hedge
{
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
    void HedgeEditorGuiManager::UpperMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open Project..."))
                {
                    std::string ymlNamePath = SelectYmlDialog();
                    raiiManager.GetHedgeEditor()->ReleaseCurrentProjectRsrc();
                    raiiManager.GetHedgeEditorRenderManager()->InitAllInUseGpuRsrc();
                    raiiManager.GetHedgeEditor()->OpenGameProject(ymlNamePath);
                    m_pRenderResultImgView = raiiManager.GetHedgeEditorRenderManager()->GetCurrentRenderImgView();
                    raiiManager.GetHedgeEditorRenderManager()->SkipThisFrame();
                }
                if (ImGui::MenuItem("Save Project to..."))
                {
                    std::string projFolderStr = SaveToFolderDialog();
                    raiiManager.GetHedgeEditor()->ReleaseCurrentProjectRsrc();
                    raiiManager.GetHedgeEditorRenderManager()->InitAllInUseGpuRsrc();
                    raiiManager.GetHedgeEditor()->GameProjectSaveAs(projFolderStr);
                    m_pRenderResultImgView = raiiManager.GetHedgeEditorRenderManager()->GetCurrentRenderImgView();
                    raiiManager.GetHedgeEditorRenderManager()->SkipThisFrame();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Edit Project Name"))
                {

                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Build"))
            {
                if (ImGui::MenuItem("Package Debug Game"))
                {
                    // Put game.exe under the project folder for debug purpose.
                    raiiManager.GetHedgeEditor()->BuildDebugGame();
                }

                if (ImGui::MenuItem("Package Release Game..."))
                {
                    // Put game.exe under the target folder for shipping.
                    std::string projFolderStr = SaveToFolderDialog();
                    raiiManager.GetHedgeEditor()->BuildAndReleaseGame(projFolderStr);
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::BottomMenuBar()
    {
        if (DearImGuiExt::BeginBottomMainMenuBar())
        {
            ImGui::EndMainMenuBar();
        }
    }

    // ================================================================================================================
    // Also execute operations triggerred by GUI.
    // NOTE: In the game template, we need to put ImGUI things into an GUI entity. 
    // Due to an existing scene for game, we don't create another scene for the world editor to hold a GUI entity.
    void HedgeEditorGuiManager::GenerateImGuiData(
        VkImageView* pResultImgView,
        VkExtent2D resultImgExtent,
        uint32_t     frameIdx)
    {
        m_frameIdx = frameIdx;
        m_pRenderResultImgView = pResultImgView;
        m_renderResultImgExtent = resultImgExtent;

        // Editor GUI
        UpperMenuBar();
        if (raiiManager.GetHedgeEditorRenderManager()->IsThisFrameSkipped() == false)
        {
            m_pLayout->BeginEndLayout();
        }
        
        BottomMenuBar();
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
    // TODO: Figure out the image four corners override the border problem. I may need to submit a fix
    void HedgeEditorGuiManager::SceneRenderWindow()
    {
        HedgeEditorGuiManager* pGui = raiiManager.GetHedgeEditorGuiManager();

        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        VkDescriptorSet my_image_texture = 0;

        if (ImGui::Begin("Scene Render Window", nullptr, flags))
        {
            pGui->AddTextureToImGUI(&my_image_texture, pGui->m_pRenderResultImgView, pGui->m_frameIdx);
            ImGui::Image((ImTextureID)my_image_texture,
                ImVec2(static_cast<float>(pGui->m_renderResultImgExtent.width),
                    static_cast<float>(pGui->m_renderResultImgExtent.height)));
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
        // ImGui::PopStyleVar(2);
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
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
        ImGui::PopStyleVar(2);
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
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
        ImGui::PopStyleVar(2);
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
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
        ImGui::PopStyleVar(2);
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
}