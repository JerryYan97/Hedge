#include "HedgeEditorRenderManager.h"
#include "HedgeEditorGuiManager.h"

namespace Hedge
{
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
}