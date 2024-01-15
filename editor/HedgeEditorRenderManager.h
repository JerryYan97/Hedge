#pragma once
#include "render/HRenderManager.h"

namespace Hedge
{
    class HedgeEditorRenderManager : public HRenderManager
    {
    public:
        HedgeEditorRenderManager(HBaseGuiManager* pGuiManager, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HedgeEditorRenderManager();

        virtual void DrawHud(HFrameListener* pFrameListener) override;

        // For the open project. Unloading the old rsrc and uploading the new rsrc.
        void ReleaseAllInUseGpuRsrc();
        void InitAllInUseGpuRsrc();

        void SkipThisFrame() { m_skipSubmitThisFrameCommandBuffer = true; }
        bool IsThisFrameSkipped() { return m_skipSubmitThisFrameCommandBuffer; }
    };
}