#include "HCubemapRendererPipeline.h"
#include "Utils.h"
#include "HRenderManager.h"
#include "../scene/HScene.h"
#include "../core/HGpuRsrcManager.h"
#include "g_prebuiltShaders.h"

namespace Hedge
{
    // ================================================================================================================
    HCubemapRendererPipeline::HCubemapRendererPipeline() :
        HPipeline()
    {}

    // ================================================================================================================
    HCubemapRendererPipeline::~HCubemapRendererPipeline()
    {}

    // ================================================================================================================
    void HCubemapRendererPipeline::CreateSetCustomPipelineInfo()
    {
        VkPipelineRenderingCreateInfoKHR pipelineRenderCreateInfo{};
        {
            pipelineRenderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderCreateInfo.colorAttachmentCount = 1;
            pipelineRenderCreateInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
        }

        VkPipelineRenderingCreateInfoKHR* pPipelineRenderCreateInfo = new VkPipelineRenderingCreateInfoKHR();
        memcpy(pPipelineRenderCreateInfo, &pipelineRenderCreateInfo, sizeof(VkPipelineRenderingCreateInfoKHR));
        SetPNext(pPipelineRenderCreateInfo);

        // Load Shader scripts and create shader modules
        VkShaderModule vertShaderModule = CreateShaderModule((uint32_t*)skybox_vertScript, sizeof(skybox_vertScript));
        VkShaderModule fragShaderModule = CreateShaderModule((uint32_t*)skybox_fragScript, sizeof(skybox_fragScript));

        AddShaderStageInfo(CreateDefaultShaderStgCreateInfo(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT));
        AddShaderStageInfo(CreateDefaultShaderStgCreateInfo(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT));

        m_shaderModules.push_back(vertShaderModule);
        m_shaderModules.push_back(fragShaderModule);

        CreateSetDescriptorSetLayouts();
        CreateSetCubemapPipelineLayout();
    }

    // ================================================================================================================
    void HCubemapRendererPipeline::CreateSetDescriptorSetLayouts()
    {
        std::vector<VkDescriptorSetLayoutBinding> skyboxDescriptorSetBindings;

        VkDescriptorSetLayoutBinding cubemapTexBinding{};
        {
            cubemapTexBinding.binding = 0;
            cubemapTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            cubemapTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            cubemapTexBinding.descriptorCount = 1;
        }
        skyboxDescriptorSetBindings.push_back(cubemapTexBinding);

        VkDescriptorSetLayoutBinding cameraUboBinding{};
        {
            cameraUboBinding.binding = 1;
            cameraUboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cameraUboBinding.descriptorCount = 1;
        }
        skyboxDescriptorSetBindings.push_back(cameraUboBinding);

        VkDescriptorSetLayoutCreateInfo skyboxDescriptorSetLayoutCreateInfo{};
        {
            skyboxDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            skyboxDescriptorSetLayoutCreateInfo.bindingCount = skyboxDescriptorSetBindings.size();
            skyboxDescriptorSetLayoutCreateInfo.pBindings = skyboxDescriptorSetBindings.data();
        }

        VkDescriptorSetLayout skyboxDescriptorSetLayout;
        VK_CHECK(vkCreateDescriptorSetLayout(m_device,
                                             &skyboxDescriptorSetLayoutCreateInfo,
                                             nullptr,
                                             &skyboxDescriptorSetLayout));

        AddDescriptorSetLayout(skyboxDescriptorSetLayout);
    }

    // ================================================================================================================
    void HCubemapRendererPipeline::CreateSetCubemapPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = m_descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
        }

        VkPipelineLayout skyboxPipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &skyboxPipelineLayout));
        SetPipelineLayout(skyboxPipelineLayout);
    }

    // ================================================================================================================
    HCubemapRenderer::HCubemapRenderer(VkDevice device)
        : HRenderer(device)
    {
        HCubemapRendererPipeline* pCubemapPipeline = new HCubemapRendererPipeline();
        pCubemapPipeline->CreatePipeline(device);
        m_pPipelines.push_back(pCubemapPipeline);
    }

    // ================================================================================================================
    HCubemapRenderer::~HCubemapRenderer()
    {}

    // ================================================================================================================
    void HCubemapRenderer::CmdRenderInsts(
        VkCommandBuffer& cmdBuf,
        const HRenderContext* const pRenderCtx,
        const SceneRenderInfo& sceneRenderInfo,
        HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl)
    {
        if (sceneRenderInfo.skyboxCubemapGpuImg)
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

            std::vector<ShaderInputBinding> bindings = GenPerFrameGpuRsrcBindings(sceneRenderInfo, pFrameGpuRsrcControl);
            m_pPipelines[0]->CmdBindDescriptors(cmdBuf, bindings);

            vkCmdDraw(cmdBuf, 6, 1, 0, 0); // 6 vertices for a screen quad.

            vkCmdEndRendering(cmdBuf);
        }
    }

    // ================================================================================================================
    std::vector<ShaderInputBinding> HCubemapRenderer::GenPerFrameGpuRsrcBindings(const SceneRenderInfo& sceneRenderInfo,
        HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl)
    {
        ShaderInputBinding cubemapTexBinding{ HGPU_IMG, 0, sceneRenderInfo.skyboxCubemapGpuImg };

        HGpuBuffer* pCameraUboBuffer = pFrameGpuRsrcControl->CreateInitTmpGpuBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            (void*)&sceneRenderInfo.cameraInfo, sizeof(sceneRenderInfo.cameraInfo));

        ShaderInputBinding cameraUboBinding{ HGPU_BUFFER, 1, pCameraUboBuffer };

        std::vector<ShaderInputBinding> bindings{ cubemapTexBinding, cameraUboBinding };

        return bindings;
    }
} // End of namespace Hedge.