#include "HRenderer.h"
#include "HRenderManager.h"
#include "../logging/HLogger.h"
#include "../scene/HScene.h"
#include "Utils.h"
#include "UtilMath.h"
#include "HPipeline.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <vector>
#include <set>

namespace Hedge
{
    // ================================================================================================================
    HRenderer::HRenderer(VkDevice device) :
        m_device(device)
    {
    }

    // ================================================================================================================
    HRenderer::~HRenderer()
    {
        for (HPipeline* pPipeline : m_pPipelines)
        {
            delete pPipeline;
        }
    }

    // ================================================================================================================
    HBasicRenderer::HBasicRenderer(VkDevice device)
        : HRenderer(device)
    {
        PBRPipeline* pPipeline = new PBRPipeline();
        pPipeline->CreatePipeline(m_device);
        m_pPipelines.push_back(pPipeline);
    }

    // ================================================================================================================
    HBasicRenderer::~HBasicRenderer()
    {
    }

    // ================================================================================================================
    HRendererInfo HBasicRenderer::GetRendererInfo()
    {

    }

    // ================================================================================================================
    void HBasicRenderer::CmdRenderInsts(
        VkCommandBuffer&            cmdBuf,
        const HRenderContext* const pRenderCtx)
    {

    }
}