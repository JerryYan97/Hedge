#include "HedgeEditorGuiManager.h"

#include "imgui.h"
#include "CustomDearImGuiLayout.h"

#include "HedgeEditor.h"
#include "Utils.h"
#include "UtilMath.h"
#include "../scene/HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"

extern Hedge::GlobalVariablesRAIIManager g_raiiManager;
extern Hedge::HFrameListener* g_pFrameListener;

namespace Hedge
{
    // ================================================================================================================
    HedgeEditorGuiManager::HedgeEditorGuiManager()
        : m_pLayout(CreateGuiLayout()),
          m_pSelectedEntity(nullptr)
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
                    g_raiiManager.GetHedgeEditor()->ReleaseCurrentProjectRsrc();
                    g_raiiManager.GetHedgeEditorRenderManager()->InitAllInUseGpuRsrc();
                    g_raiiManager.GetHedgeEditor()->OpenGameProject(ymlNamePath);
                    m_pRenderResultImgView = g_raiiManager.GetHedgeEditorRenderManager()->GetCurrentRenderImgView();
                    g_raiiManager.GetHedgeEditorRenderManager()->SkipThisFrame();
                }
                if (ImGui::MenuItem("Save Project to..."))
                {
                    std::string projFolderStr = SaveToFolderDialog();
                    g_raiiManager.GetHedgeEditor()->ReleaseCurrentProjectRsrc();
                    g_raiiManager.GetHedgeEditorRenderManager()->InitAllInUseGpuRsrc();
                    g_raiiManager.GetHedgeEditor()->GameProjectSaveAs(projFolderStr);
                    m_pRenderResultImgView = g_raiiManager.GetHedgeEditorRenderManager()->GetCurrentRenderImgView();
                    g_raiiManager.GetHedgeEditorRenderManager()->SkipThisFrame();
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
                    g_raiiManager.GetHedgeEditor()->BuildDebugGame();
                }

                if (ImGui::MenuItem("Package Release Game..."))
                {
                    // Put game.exe under the target folder for shipping.
                    std::string projFolderStr = SaveToFolderDialog();
                    g_raiiManager.GetHedgeEditor()->BuildAndReleaseGame(projFolderStr);
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
        if (g_raiiManager.GetHedgeEditorRenderManager()->IsThisFrameSkipped() == false)
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
        HedgeEditorGuiManager* pGui = g_raiiManager.GetHedgeEditorGuiManager();

        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        VkDescriptorSet my_image_texture = 0;

        if (ImGui::Begin("Scene Render Window", nullptr, flags))
        {
            pGui->AddTextureToImGUI(&my_image_texture, pGui->m_pRenderResultImgView, pGui->m_frameIdx);
            ImGui::Image((ImTextureID)my_image_texture,
                ImVec2(static_cast<float>(pGui->m_renderResultImgExtent.width),
                    static_cast<float>(pGui->m_renderResultImgExtent.height)));
        }
        ImGui::End();
        // ImGui::PopStyleVar(3);
        ImGui::PopStyleVar(2);
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::AssetWindow()
    {
        constexpr ImGuiWindowFlags TestWindowFlag = ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration;

        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("AssetWindow", nullptr, TestWindowFlag);
        
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Assets"))
            {

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        /*
        if (ImGui::TreeNode("Assets"))
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
        */
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

        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("SceneObjectsListWindow", nullptr, TestWindowFlag);

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);

        HScene& scene = g_pFrameListener->GetActiveScene();
        std::vector<std::pair<std::string, uint32_t>> nameHashVec;
        scene.GetAllEntitiesNamesHashes(nameHashVec);

        HedgeEditorGuiManager* pEditorGuiManager = g_raiiManager.GetHedgeEditorGuiManager();

        if (ImGui::TreeNodeEx("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < nameHashVec.size(); i++)
            {
                std::string sceneInstStr;
                sceneInstStr = nameHashVec[i].first + " (" + std::to_string(nameHashVec[i].second) + ")";

                if (ImGui::Selectable(sceneInstStr.c_str(), pEditorGuiManager->m_selectedId == i))
                {
                    pEditorGuiManager->m_selectedId = i;
                    pEditorGuiManager->m_pSelectedEntity = scene.GetEntity(nameHashVec[i].second);
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

        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("ObjectPropertiesWindow", nullptr, TestWindowFlag);

        HedgeEditorGuiManager* pEditorGuiManager = g_raiiManager.GetHedgeEditorGuiManager();

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Properties"))
            {
                if (pEditorGuiManager->m_pSelectedEntity != nullptr)
                {
                    DrawProperties(pEditorGuiManager->m_pSelectedEntity);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
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
    void HedgeEditorGuiManager::DrawProperties(
        HEntity* pEntity)
    {
        std::vector<uint32_t> componentsName;
        pEntity->GetComponentsNamesHashes(componentsName);

        for (uint32_t componentNameHash : componentsName)
        {
            switch (componentNameHash) {
            case crc32("TransformComponent"):
                DrawTransformComponentProperties(pEntity);
                break;
            case crc32("StaticMeshComponent"):
                DrawStaticMeshComponentProperties(pEntity);
                break;
            case crc32("CameraComponent"):
                DrawCameraComponentProperties(pEntity);
                break;
            case crc32("PointLightComponent"):
                DrawPointLightComponentProperties(pEntity);
                break;
            case crc32("ImageBasedLightingComponent"):
                DrawImageBasedLightingComponentProperties(pEntity);
                break;
            default:
                break;
            }
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::DrawTransformComponentProperties(
        HEntity* pEntity)
    {
        if (ImGui::TreeNodeEx("Transform Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            HScene& scene = g_pFrameListener->GetActiveScene();

            TransformComponent transComponent = scene.EntityGetComponent<TransformComponent>(pEntity->GetEntityHandle());

            std::string posStr;
            posStr += ("(" + std::to_string(transComponent.m_pos[0]) + ", " +
                             std::to_string(transComponent.m_pos[1]) + ", " +
                             std::to_string(transComponent.m_pos[2]) + ")");
            ImGui::Text("Translation: ");
            ImGui::SameLine();
            ImGui::Text(posStr.c_str());

            std::string rotStr;
            rotStr += ("(" + std::to_string(transComponent.m_rot[0]) + ", " +
                             std::to_string(transComponent.m_rot[1]) + ", " +
                             std::to_string(transComponent.m_rot[2]) + ")");
            ImGui::Text("Rotation: ");
            ImGui::SameLine();
            ImGui::Text(rotStr.c_str());
            
            std::string scaleStr;
            scaleStr += ("(" + std::to_string(transComponent.m_scale[0]) + ", " +
                               std::to_string(transComponent.m_scale[1]) + ", " +
                               std::to_string(transComponent.m_scale[2]) + ")");
            ImGui::Text("Scale: ");
            ImGui::SameLine();
            ImGui::Text(scaleStr.c_str());

            ImGui::TreePop();
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::DrawStaticMeshComponentProperties(
        HEntity* pEntity)
    {
        if (ImGui::TreeNodeEx("Static Mesh Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            ImGui::TreePop();
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::DrawCameraComponentProperties(
        HEntity* pEntity)
    {
        if (ImGui::TreeNodeEx("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            ImGui::TreePop();
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::DrawPointLightComponentProperties(
        HEntity* pEntity)
    {
        if (ImGui::TreeNodeEx("Point Light Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            ImGui::TreePop();
        }
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::DrawImageBasedLightingComponentProperties(
        HEntity* pEntity)
    {
        if (ImGui::TreeNodeEx("Image Based Lighting Component", ImGuiTreeNodeFlags_DefaultOpen))
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            ImGui::TreePop();
        }
    }
}