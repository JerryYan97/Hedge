#pragma once

#include "core/HFrameListener.h"
#include "render/HBaseGuiManager.h"
#include "render/HRenderManager.h"
#include "core/HGpuRsrcManager.h"
#include <vector>

namespace DearImGuiExt
{
    class CustomLayout;
    class CustomLayoutNode;
};

namespace Hedge
{
    class HScene;

    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        virtual ~HedgeEditor();

        void BuildGame(const char* pPathFileName);

        void CreateGameProject(const char* pPath) {};

        void Run();

        virtual void FrameStarted() override;
        virtual void FrameEnded() override;

        virtual HScene& GetActiveScene() override;

    private:
        std::vector<HScene*> m_pScenes;
        uint32_t             m_activeScene;
    };

    class HedgeEditorRenderManager : public HRenderManager
    {
    public:
        HedgeEditorRenderManager(HBaseGuiManager* pGuiManager, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HedgeEditorRenderManager();

        virtual void DrawHud(HFrameListener* pFrameListener) override;
    };

    class HedgeEditorGuiManager : public HBaseGuiManager
    {
    public:
        HedgeEditorGuiManager();
        virtual ~HedgeEditorGuiManager();

        virtual void GenerateImGuiData() override {};
        void GenerateImGuiData(VkImageView* resultImgView, VkExtent2D resultImgExtent, uint32_t frameIdx);

        virtual VkExtent2D GetRenderExtent() override;

        DearImGuiExt::CustomLayout* CreateGuiLayout();

    private:
        static void SceneRenderWindow();
        static void AssetWindow();
        static void SceneObjectsListWindow();
        static void ObjectPropertiesWindow();

        VkImageView*                m_pRenderResultImgView;
        VkExtent2D                  m_renderResultImgExtent;
        uint32_t                    m_frameIdx;
        DearImGuiExt::CustomLayout* m_pLayout;
        DearImGuiExt::CustomLayoutNode* m_pRenderWindowNode;
    };

    class GlobalVariablesRAIIManager
    {
    public:
        GlobalVariablesRAIIManager();
        ~GlobalVariablesRAIIManager();

        HedgeEditor* GetHedgeEditor() { return m_pHedgeEditor; }
        HedgeEditorRenderManager* GetHedgeEditorRenderManager() { return m_pHedgeEditorRenderManager; }
        HedgeEditorGuiManager* GetHedgeEditorGuiManager() { return m_pHedgeEditorGuiManager; }
        HGpuRsrcManager* GetGpuRsrcManager() { return m_pGpuRsrcManager; }

    private:
        HedgeEditor*              m_pHedgeEditor;
        HedgeEditorGuiManager*    m_pHedgeEditorGuiManager;
        HedgeEditorRenderManager* m_pHedgeEditorRenderManager;
        HGpuRsrcManager*          m_pGpuRsrcManager;
    };
}
