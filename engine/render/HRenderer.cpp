#include "HRenderer.h"
#include "HRenderManager.h"
#include "../logging/HLogger.h"
#include "../scene/HScene.h"
#include "Utils.h"
#include "UtilMath.h"
#include "../core/HGpuRsrcManager.h"

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
    void HRenderer::CmdTransImgLayout(
        VkCommandBuffer& cmdBuf,
        HGpuImg* pGpuImg,
        VkImageLayout targetLayout,
        VkAccessFlags srcFlags,
        VkAccessFlags dstFlags,
        VkPipelineStageFlags srcPipelineStg,
        VkPipelineStageFlags dstPipelineStg)
    {
        VkImageMemoryBarrier toTargetBarrier{};
        {
            toTargetBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toTargetBarrier.image = pGpuImg->gpuImg;
            toTargetBarrier.subresourceRange = pGpuImg->imgSubresRange;
            toTargetBarrier.srcAccessMask = srcFlags;
            toTargetBarrier.dstAccessMask = dstFlags;
            toTargetBarrier.oldLayout = pGpuImg->curImgLayout;
            toTargetBarrier.newLayout = targetLayout;
        }

        vkCmdPipelineBarrier(
            cmdBuf,
            srcPipelineStg,
            dstPipelineStg,
            0,
            0, nullptr,
            0, nullptr,
            1, &toTargetBarrier);

        pGpuImg->curImgLayout = targetLayout;
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
    void HBasicRenderer::CmdRenderInsts(
        VkCommandBuffer&            cmdBuf,
        const HRenderContext* const pRenderCtx)
    {
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

        VkRenderingAttachmentInfoKHR renderColorAttachmentInfo{};
        {
            renderColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            renderColorAttachmentInfo.imageView = pRenderCtx->pColorAttachmentImg->gpuImgView;
            renderColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            renderColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            renderColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            renderColorAttachmentInfo.clearValue = clearColor;
        }

        VkClearValue depthClearVal{};
        depthClearVal.depthStencil.depth = 0.f;
        VkRenderingAttachmentInfoKHR depthModelAttachmentInfo{};
        {
            depthModelAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            depthModelAttachmentInfo.imageView = pRenderCtx->pDepthAttachmentImg->gpuImgView;
            depthModelAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthModelAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthModelAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthModelAttachmentInfo.clearValue = depthClearVal;
        }

        VkRenderingInfoKHR renderInfo{};
        {
            renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderInfo.renderArea = pRenderCtx->renderArea;
            renderInfo.layerCount = 1;
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachments = &renderColorAttachmentInfo;
            renderInfo.pDepthAttachment = &depthModelAttachmentInfo;
        }

        vkCmdBeginRendering(cmdBuf, &renderInfo);

        m_pPipelines[0]->CmdBindDescriptors(cmdBuf, pRenderCtx->bindings);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelines[0]->GetVkPipeline());

        VkViewport viewport{};
        {
            viewport.x = 0.f;
            viewport.y = 0.f;
            viewport.width = pRenderCtx->renderArea.extent.width;
            viewport.height = pRenderCtx->renderArea.extent.height;
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;
        }
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        VkRect2D scissor{};
        {
            scissor.offset = { 0, 0 };
            scissor.extent = pRenderCtx->renderArea.extent;
        }
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        VkDeviceSize vbOffset = 0;
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &pRenderCtx->pVertBuffer->gpuBuffer, &vbOffset);
        vkCmdBindIndexBuffer(cmdBuf, pRenderCtx->pIdxBuffer->gpuBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(cmdBuf,
                           m_pPipelines[0]->GetVkPipelineLayout(),
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           pRenderCtx->pushConstantDataBytesCnt,
                           pRenderCtx->pPushConstantData);
        vkCmdDrawIndexed(cmdBuf, pRenderCtx->idxCnt, 1, 0, 0, 0);
        vkCmdEndRendering(cmdBuf);
    }
}