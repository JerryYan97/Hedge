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
    /*
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
    */

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
    std::vector<ShaderInputBinding> HBasicRenderer::GenPerFrameGpuRsrcBinding(
        const SceneRenderInfo&      sceneRenderInfo,
        HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl)
    {
        // The point light data
        uint32_t pointLightPosRadianceBytesCnt = sizeof(HVec3) * sceneRenderInfo.pointLightsPositions.size();

        HGpuBuffer* pPtLightsPosStorageBuffer = pFrameGpuRsrcControl->CreateInitTmpGpuBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            (void*)sceneRenderInfo.pointLightsPositions.data(), pointLightPosRadianceBytesCnt
        );

        ShaderInputBinding ptLightsPosBinding{ HGPU_BUFFER, 8, pPtLightsPosStorageBuffer };

        HGpuBuffer* pPtLightsRadianceStorageBuffer = pFrameGpuRsrcControl->CreateInitTmpGpuBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            (void*)sceneRenderInfo.pointLightsRadiances.data(), pointLightPosRadianceBytesCnt
        );

        ShaderInputBinding ptLightsRadianceBinding{ HGPU_BUFFER, 9, pPtLightsRadianceStorageBuffer };

        // Image based lightning bindings
        // Diffuse cubemap light map
        ShaderInputBinding diffuseLightCubemapBinding{ HGPU_IMG, 1, (void*)sceneRenderInfo.diffuseCubemapGpuImg };

        // Prefilter environment cubemap
        ShaderInputBinding prefilterEnvCubemapBinding{ HGPU_IMG, 2, (void*)sceneRenderInfo.prefilterEnvCubemapGpuImg };

        // Environment BRDF
        ShaderInputBinding envBrdfBinding{ HGPU_IMG, 3, (void*)sceneRenderInfo.envBrdfGpuImg };

        std::vector<ShaderInputBinding> perFrameBindings{ ptLightsPosBinding,
                                                          ptLightsRadianceBinding,
                                                          diffuseLightCubemapBinding,
                                                          prefilterEnvCubemapBinding,
                                                          envBrdfBinding };
        
        return perFrameBindings;
    }

    // ================================================================================================================
    std::vector<ShaderInputBinding> HBasicRenderer::GenPerObjGpuRsrcBinding(
        const SceneRenderInfo&      sceneRenderInfo,
        HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl,
        uint32_t                    objIdx)
    {
        // The model matrix and view-perspective matrix UBO data.
        uint32_t vertUboDataBytesCnt = 32 * sizeof(float);
        void* pVertUboData = malloc(vertUboDataBytesCnt);

        HMat4x4 modelMat = sceneRenderInfo.modelMats[objIdx];
        HMat4x4 vpMat = sceneRenderInfo.vpMat;

        memcpy(pVertUboData, modelMat.eles, sizeof(HMat4x4));
        memcpy(static_cast<char*>(pVertUboData) + sizeof(HMat4x4), vpMat.eles, sizeof(HMat4x4));

        HGpuBuffer* pVertUbo = pFrameGpuRsrcControl->CreateInitTmpGpuBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
                                                                            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                                            pVertUboData, vertUboDataBytesCnt);

        free(pVertUboData);
        ShaderInputBinding vertUboBinding{ HGPU_BUFFER, 0, pVertUbo };

        // Base color
        ShaderInputBinding baseColorBinding{ HGPU_IMG, 4, (void*)sceneRenderInfo.modelBaseColors[objIdx] };

        // Normal map
        ShaderInputBinding normalMapBinding{ HGPU_IMG, 5, (void*)sceneRenderInfo.modelNormalTexs[objIdx] };

        // Metallic roughness
        ShaderInputBinding metallicRoughnessBinding{ HGPU_IMG, 6, (void*)sceneRenderInfo.modelMetallicRoughnessTexs[objIdx] };

        // Occlusion
        ShaderInputBinding occlusionBinding{ HGPU_IMG, 7, (void*)sceneRenderInfo.modelOcclusionTexs[objIdx] };

        std::vector<ShaderInputBinding> perObjBindings{ vertUboBinding,
                                                        baseColorBinding,
                                                        normalMapBinding,
                                                        metallicRoughnessBinding,
                                                        occlusionBinding };

        return perObjBindings;
    }

    // ================================================================================================================
    void* HBasicRenderer::GenPushConstants(
        const SceneRenderInfo& sceneRenderInfo,
        uint32_t&              bytesCnt)
    {
        // The scene information data.
        uint32_t fragPushConstantDataBytesCnt = sizeof(float) * 4 + sizeof(uint32_t);
        void* pFragPushConstantData = malloc(fragPushConstantDataBytesCnt);
        memset(pFragPushConstantData, 0, fragPushConstantDataBytesCnt);
        memcpy(pFragPushConstantData, sceneRenderInfo.cameraPos, sizeof(float) * 3);

        uint32_t ptLightsCnt = sceneRenderInfo.pointLightsPositions.size();
        memcpy(static_cast<char*>(pFragPushConstantData) + sizeof(float) * 4, &ptLightsCnt, sizeof(ptLightsCnt));

        bytesCnt = fragPushConstantDataBytesCnt;

        return pFragPushConstantData;
    }

    // ================================================================================================================
    void HBasicRenderer::CmdRenderInsts(
        VkCommandBuffer& cmdBuf,
        const HRenderContext* const pRenderCtx,
        const SceneRenderInfo& sceneRenderInfo,
        HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl)
    {
        std::vector<ShaderInputBinding> perFrameGpuRsrcBindings = GenPerFrameGpuRsrcBinding(sceneRenderInfo,
                                                                                            pFrameGpuRsrcControl);
        uint32_t pushConstantBytesCnt = 0;
        void* pPushConstantData = GenPushConstants(sceneRenderInfo, pushConstantBytesCnt);

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

        uint32_t objsCnt = sceneRenderInfo.modelMats.size();
        if (objsCnt != 0)
        {
            for (uint32_t objIdx = 0; objIdx < objsCnt; objIdx++)
            {
                std::vector<ShaderInputBinding> perObjGpuRsrcBindings = GenPerObjGpuRsrcBinding(sceneRenderInfo,
                                                                                                pFrameGpuRsrcControl,
                                                                                                objIdx);

                std::vector<ShaderInputBinding> bindings = perFrameGpuRsrcBindings;
                bindings.insert(bindings.end(), perObjGpuRsrcBindings.begin(), perObjGpuRsrcBindings.end());

                vkCmdBeginRendering(cmdBuf, &renderInfo);

                m_pPipelines[0]->CmdBindDescriptors(cmdBuf, bindings);

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
                vkCmdBindVertexBuffers(cmdBuf, 0, 1, &sceneRenderInfo.objsVertBuffers[objIdx]->gpuBuffer, &vbOffset);
                vkCmdBindIndexBuffer(cmdBuf, sceneRenderInfo.objsIdxBuffers[objIdx]->gpuBuffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdPushConstants(cmdBuf,
                    m_pPipelines[0]->GetVkPipelineLayout(),
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    pushConstantBytesCnt,
                    pPushConstantData);
                vkCmdDrawIndexed(cmdBuf, sceneRenderInfo.idxCounts[objIdx], 1, 0, 0, 0);
                vkCmdEndRendering(cmdBuf);

                // Add the static mesh gpu rsrc into the frame resource control
                pFrameGpuRsrcControl->AddGpuBufferReferControl(sceneRenderInfo.objsIdxBuffers[objIdx]);
                pFrameGpuRsrcControl->AddGpuBufferReferControl(sceneRenderInfo.objsVertBuffers[objIdx]);
                pFrameGpuRsrcControl->AddGpuImgReferControl(sceneRenderInfo.modelBaseColors[objIdx]);
                pFrameGpuRsrcControl->AddGpuImgReferControl(sceneRenderInfo.modelNormalTexs[objIdx]);
                pFrameGpuRsrcControl->AddGpuImgReferControl(sceneRenderInfo.modelMetallicRoughnessTexs[objIdx]);
                pFrameGpuRsrcControl->AddGpuImgReferControl(sceneRenderInfo.modelOcclusionTexs[objIdx]);
            }
        }

        free(pPushConstantData);
    }
}