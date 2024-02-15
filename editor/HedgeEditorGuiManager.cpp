#include "HedgeEditorGuiManager.h"

#include "imgui.h"
#include "CustomDearImGuiLayout.h"

#include "HedgeEditor.h"
#include "Utils.h"
#include "UtilMath.h"
#include "../scene/HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"

#include "stb_image.h"

extern Hedge::GlobalVariablesRAIIManager g_raiiManager;
extern Hedge::HFrameListener* g_pFrameListener;
extern Hedge::HGpuRsrcManager* g_pGpuRsrcManager;

namespace Hedge
{
    // ================================================================================================================
    HedgeEditorGuiManager::HedgeEditorGuiManager()
        : m_pLayout(CreateGuiLayout()),
          m_pSelectedEntity(nullptr),
          m_pAssetIconImg(nullptr),
          m_assetIconDescSet(VK_NULL_HANDLE)
    {}

    // ================================================================================================================
    HedgeEditorGuiManager::~HedgeEditorGuiManager()
    {
        g_pGpuRsrcManager->DereferGpuImg(m_pAssetIconImg);
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
        
        HedgeEditorGuiManager* pEditorGuiManager = g_raiiManager.GetHedgeEditorGuiManager();

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Assets"))
            {
                for (int i = 0; i < 8; i++)
                {
                    // UV coordinates are often (0.0f, 0.0f) and (1.0f, 1.0f) to display an entire textures.
                    // Here are trying to display only a 32x32 pixels area of the texture, hence the UV computation.
                    // Read about UV coordinates here: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
                    ImGui::PushID(i);
                    if (i > 0)
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(i - 1.0f, i - 1.0f));
                    }

                    ImVec2 size = ImVec2(64.0f, 64.0f);                         // Size of the image we want to make visible
                    ImVec2 uv0 = ImVec2(0.0f, 0.0f);                            // UV coordinates for lower-left
                    ImVec2 uv1 = ImVec2(1.f, 1.f);    // UV coordinates for (32,32) in our texture
                    ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // Black background
                    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);           // No tint
                    if (ImGui::ImageButton("", (ImTextureID)pEditorGuiManager->m_assetIconDescSet, size, uv0, uv1, bg_col, tint_col))
                    {
                        std::cout << "Asset button pushed" << std::endl;
                    }
                    if (i > 0)
                    {
                        ImGui::PopStyleVar();
                    }
                    ImGui::PopID();
                    ImGui::SameLine();
                }

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

    // ================================================================================================================
    void HedgeEditorGuiManager::AppStart()
    {
        LoadEditorGuiRsrc();
    }

    // ================================================================================================================
    void HedgeEditorGuiManager::LoadEditorGuiRsrc()
    {
        std::string editorRsrcDir(getenv("HEDGE_LIB"));
        editorRsrcDir += "\\EditorRsrcs";

        std::string editorAssetIconAssetName = editorRsrcDir + "\\AssetIcon.png";

        int iconWidth = 0;
        int iconHeight = 0;
        int iconComponent = 0;

        unsigned char* iconData = stbi_load(editorAssetIconAssetName.c_str(),
            &iconWidth, &iconHeight, &iconComponent, 0);

        HGpuImgCreateInfo iconImgCreateInfo{};
        {
            iconImgCreateInfo.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            iconImgCreateInfo.hasSampler = true;
            iconImgCreateInfo.samplerInfo = Util::LinearRepeatSamplerInfo();
            iconImgCreateInfo.imgExtent = Util::Depth1Extent3D(iconWidth, iconHeight);
            iconImgCreateInfo.imgFormat = VK_FORMAT_R8G8B8A8_UNORM;
            iconImgCreateInfo.imgSubresRange = Util::ImgSubRsrcRangeTexColor2D();
            iconImgCreateInfo.imgUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            iconImgCreateInfo.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
        }
        m_pAssetIconImg = g_pGpuRsrcManager->CreateGpuImage(iconImgCreateInfo, "Editor Asset Icon PNG");

        g_pGpuRsrcManager->SendDataToImage(m_pAssetIconImg,
            Util::BufferImg2DCopy(iconWidth, iconHeight),
            (void*)iconData, iconWidth * iconHeight * 4);

        g_pGpuRsrcManager->TransImageLayout(m_pAssetIconImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        AddTextureToImGUI(&m_assetIconDescSet, m_pAssetIconImg);
    }
}