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
    };
}