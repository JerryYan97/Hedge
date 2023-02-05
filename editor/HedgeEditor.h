#pragma once

#include "core/HFrameListener.h"
#include "render/HBaseGuiManager.h"
#include "render/HRenderManager.h"
#include <vector>

namespace DearImGuiExt
{
    class CustomLayout;
};

namespace Hedge
{
    class HScene;

    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        ~HedgeEditor();

        void BuildGame(const char* pPathFileName);

        void CreateGameProject(const char* pPath) {};

        void Run();

        virtual void FrameStarted() override;
        virtual void FrameEnded() override;

        virtual HScene& GetActiveScene() override;

    private:
        DearImGuiExt::CustomLayout* m_pLayout;
        std::vector<HScene*> m_pScenes;
        uint32_t m_activeScene;
    };

    class HedgeEditorRenderManager : public HRenderManager
    {
    public:
        HedgeEditorRenderManager(HBaseGuiManager* pGuiManager);
        ~HedgeEditorRenderManager();

        virtual void DrawHud(HFrameListener* pFrameListener) override;
    };

    class HedgeEditorGuiManager : public HBaseGuiManager
    {
    public:
        HedgeEditorGuiManager();
        ~HedgeEditorGuiManager();

        virtual void GenerateImGuiData() override {};
        void GenerateImGuiData(VkImageView* resultImgView);

    private:
        
    };
}
