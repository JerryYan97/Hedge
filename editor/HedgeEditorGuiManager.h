#pragma once
#include "render/HBaseGuiManager.h"

namespace DearImGuiExt
{
    class CustomLayout;
    class CustomLayoutNode;
};

namespace Hedge
{
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

    private:
        static void SceneRenderWindow();
        static void AssetWindow();
        static void SceneObjectsListWindow();
        static void ObjectPropertiesWindow();

        // GUI
        void UpperMenuBar();
        void BottomMenuBar();

        // Utility
        void PackageProject(); // Build the project to a game.


        VkImageView* m_pRenderResultImgView;
        VkExtent2D                  m_renderResultImgExtent;
        uint32_t                    m_frameIdx;
        DearImGuiExt::CustomLayout* m_pLayout;
        DearImGuiExt::CustomLayoutNode* m_pRenderWindowNode;
    };
}