#pragma once
#include "render/HBaseGuiManager.h"

namespace DearImGuiExt
{
    class CustomLayout;
    class CustomLayoutNode;
};

namespace Hedge
{
    class HEntity;

    class HedgeEditorGuiManager : public HBaseGuiManager
    {
    public:
        HedgeEditorGuiManager();
        virtual ~HedgeEditorGuiManager();

        virtual void GenerateImGuiData() override {};
        void GenerateImGuiData(VkImageView* resultImgView, VkExtent2D resultImgExtent, uint32_t frameIdx);

        virtual VkExtent2D GetRenderExtent() override;

        DearImGuiExt::CustomLayout* CreateGuiLayout();

        virtual void SendIOEvents(HScene& scene, HEventManager& eventManager) override;

        virtual void AppStart() override;

    protected:        

    private:
        static void SceneRenderWindow();
        static void AssetWindow();
        static void SceneObjectsListWindow();
        static void ObjectPropertiesWindow();

        static void DrawProperties(HEntity* pEntity);
        static void DrawTransformComponentProperties(HEntity* pEntity);
        static void DrawStaticMeshComponentProperties(HEntity* pEntity);
        static void DrawCameraComponentProperties(HEntity* pEntity);
        static void DrawPointLightComponentProperties(HEntity* pEntity);
        static void DrawImageBasedLightingComponentProperties(HEntity* pEntity);

        void LoadEditorGuiRsrc();

        // GUI
        void UpperMenuBar();
        void BottomMenuBar();

        VkImageView* m_pRenderResultImgView;
        VkExtent2D                  m_renderResultImgExtent;
        uint32_t                    m_frameIdx;
        DearImGuiExt::CustomLayout* m_pLayout;
        DearImGuiExt::CustomLayoutNode* m_pRenderWindowNode;

        HEntity* m_pSelectedEntity;
        int m_selectedId;

        HGpuImg* m_pAssetIconImg;
        VkDescriptorSet m_assetIconDescSet;

        HGpuImg* m_pFolderIconImg;
        VkDescriptorSet m_folderIconDescSet;

        std::string m_currentAssetDir;
    };
}