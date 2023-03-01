#include "HRenderer.h"
#include "HRenderManager.h"
#include "../logging/HLogger.h"
#include "../scene/HScene.h"
#include "Utils.h"
#include "UtilMath.h"

#include "g_prebuiltShaders.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <vector>
#include <set>

namespace Hedge
{
    // ================================================================================================================
    HRenderer::HRenderer(
        uint32_t onFlightResCnt, 
        VkDevice* pVkDevice, 
        VkFormat surfFormat, 
        VmaAllocator* pVmaAllocator)
        : m_onFlightResCnt(onFlightResCnt),
          m_pVkDevice(pVkDevice),
          m_renderSurfFormat(surfFormat),
          m_pVmaAllocator(pVmaAllocator)
    {

    }

    // ================================================================================================================
    HRenderer::~HRenderer()
    {

    }

    // ================================================================================================================
    void HRenderer::CreateShaderModule(
        VkShaderModule* pShaderModule, 
        uint32_t*       pShaderScript, 
        uint32_t        scriptByteCnt)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        {
            shaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCreateInfo.codeSize = scriptByteCnt;
            shaderModuleCreateInfo.pCode    = pShaderScript;
        }
        VK_CHECK(vkCreateShaderModule(*m_pVkDevice, &shaderModuleCreateInfo, nullptr, pShaderModule));
    }

    // ================================================================================================================
    HBasicRenderer::HBasicRenderer(
        uint32_t onFlightResCnt, 
        VkDevice* pVkDevice, 
        VkFormat surfFormat, 
        VmaAllocator* pVmaAllocator,
        HGpuRsrcManager* pGpuRsrcManager)
        : HRenderer(onFlightResCnt, pVkDevice, surfFormat, pVmaAllocator),
          m_pGpuRsrcManager(pGpuRsrcManager)
    {
        // Create Shader Modules
        CreateShaderModule(&m_shaderVertModule, (uint32_t*)BasicRendererVertScript, sizeof(BasicRendererVertScript));
        CreateShaderModule(&m_shaderFragModule, (uint32_t*)BasicRendererFragScript, sizeof(BasicRendererFragScript));

        // Create Vert Shader Stage create info
        VkPipelineShaderStageCreateInfo vertStgInfo{};
        {
            vertStgInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertStgInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertStgInfo.pName = "main";
            vertStgInfo.module = m_shaderVertModule;
        }

        // Create frag shader stage create info
        VkPipelineShaderStageCreateInfo fragStgInfo{};
        {
            fragStgInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragStgInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragStgInfo.pName = "main";
            fragStgInfo.module = m_shaderFragModule;
        }

        // Combine shader stages info into an array
        VkPipelineShaderStageCreateInfo stgArray[] = { vertStgInfo, fragStgInfo };

        // Specifying all kinds of pipeline states
        // Vertex input state
        VkVertexInputBindingDescription vertBindingDesc = {};
        {
            vertBindingDesc.binding = 0;
            vertBindingDesc.stride = VertFloatNum * sizeof(float);
            vertBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        VkVertexInputAttributeDescription vertAttrDesc[3];
        {
            // Position
            vertAttrDesc[0].location = 0;
            vertAttrDesc[0].binding = 0;
            vertAttrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertAttrDesc[0].offset = 0;
            // Normal
            vertAttrDesc[1].location = 1;
            vertAttrDesc[1].binding = 0;
            vertAttrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertAttrDesc[1].offset = 3 * sizeof(float);
            // Uv
            vertAttrDesc[2].location = 2;
            vertAttrDesc[2].binding = 0;
            vertAttrDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
            vertAttrDesc[2].offset = 6 * sizeof(float);
        }
        VkPipelineVertexInputStateCreateInfo vertInputInfo{};
        {
            vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertInputInfo.pNext = nullptr;
            vertInputInfo.vertexBindingDescriptionCount = 1;
            vertInputInfo.pVertexBindingDescriptions = &vertBindingDesc;
            vertInputInfo.vertexAttributeDescriptionCount = 3;
            vertInputInfo.pVertexAttributeDescriptions = vertAttrDesc;
        }

        // Vertex assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        {
            inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
            inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        {
            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizationInfo.depthClampEnable = VK_FALSE;
            rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
            rasterizationInfo.depthBiasEnable = VK_FALSE;
            rasterizationInfo.depthBiasClamp = 0;
            rasterizationInfo.depthBiasSlopeFactor = 0;
            rasterizationInfo.lineWidth = 1.f;
        }

        // Color blend state
        VkPipelineColorBlendAttachmentState cbAttState{};
        {
            cbAttState.colorWriteMask = 0xf;
            cbAttState.blendEnable = VK_FALSE;
            cbAttState.alphaBlendOp = VK_BLEND_OP_ADD;
            cbAttState.colorBlendOp = VK_BLEND_OP_ADD;
            cbAttState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            cbAttState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            cbAttState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            cbAttState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        }
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        {
            colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendInfo.attachmentCount = 1;
            colorBlendInfo.pAttachments = &cbAttState;
            colorBlendInfo.logicOpEnable = VK_FALSE;
            colorBlendInfo.blendConstants[0] = 1.f;
            colorBlendInfo.blendConstants[1] = 1.f;
            colorBlendInfo.blendConstants[2] = 1.f;
            colorBlendInfo.blendConstants[3] = 1.f;
        }

        // Viewport state
        VkPipelineViewportStateCreateInfo vpStateInfo{};
        {
            vpStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vpStateInfo.viewportCount = 1;
            vpStateInfo.scissorCount = 1;
        }

        // Depth stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        {
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilInfo.depthTestEnable = VK_TRUE;
            depthStencilInfo.depthWriteEnable = VK_TRUE;
            depthStencilInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // Reverse depth for higher precision. 
            depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            depthStencilInfo.stencilTestEnable = VK_FALSE;
        }

        // Multisample state
        VkPipelineMultisampleStateCreateInfo multiSampleInfo{};
        {
            multiSampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multiSampleInfo.sampleShadingEnable = VK_FALSE;
            multiSampleInfo.alphaToCoverageEnable = VK_FALSE;
            multiSampleInfo.alphaToOneEnable = VK_FALSE;
        }

        // Create the dynamic state info for scissor and viewport
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        {
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();
        }
        
        // Create a descriptor set layout for binding mvp matrices
        VkDescriptorSetLayoutBinding uboMvpLayoutBinding{};
        {
            uboMvpLayoutBinding.binding = 0;
            uboMvpLayoutBinding.descriptorCount = 1;
            uboMvpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboMvpLayoutBinding.pImmutableSamplers = nullptr;
            uboMvpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        }

        // Create a descriptor set layout for binding light
        VkDescriptorSetLayoutBinding uboLightLayoutBinding{};
        {
            uboLightLayoutBinding.binding = 1;
            uboLightLayoutBinding.descriptorCount = 1;
            uboLightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLightLayoutBinding.pImmutableSamplers = nullptr;
            uboLightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutBinding bindingInfoArray[2] = { uboMvpLayoutBinding, uboLightLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        {
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 2;
            layoutInfo.pBindings = bindingInfoArray;
        }

        VK_CHECK(vkCreateDescriptorSetLayout(*m_pVkDevice, &layoutInfo, nullptr, &m_descriptorSetLayout));

        // Allocate the descriptor set from the descriptor pool.
        VkDescriptorPool* pDescriptorPool = m_pGpuRsrcManager->GetDescriptorPool();
        m_uboDescriptorSets.resize(onFlightResCnt);

        VkDescriptorSetLayout desciptorSetLayouts[2] = { m_descriptorSetLayout, m_descriptorSetLayout };
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        {
            descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocInfo.descriptorPool = *pDescriptorPool;
            descriptorSetAllocInfo.descriptorSetCount = onFlightResCnt;
            descriptorSetAllocInfo.pSetLayouts = desciptorSetLayouts;
        }

        vkAllocateDescriptorSets(*m_pVkDevice, &descriptorSetAllocInfo, m_uboDescriptorSets.data());

        // Create a pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
        }
        VK_CHECK(vkCreatePipelineLayout(*m_pVkDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

        // Create the graphics pipeline -- The graphics pipeline is used for scene rendering
        VkPipelineRenderingCreateInfoKHR pipelineRenderCreateInfo{};
        {
            pipelineRenderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderCreateInfo.colorAttachmentCount = 1;
            pipelineRenderCreateInfo.pColorAttachmentFormats = &surfFormat;
            pipelineRenderCreateInfo.depthAttachmentFormat = VK_FORMAT_D16_UNORM;
        }

        // Create the graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        {
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.pNext = &pipelineRenderCreateInfo;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = stgArray;
            pipelineInfo.pVertexInputState = &vertInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
            pipelineInfo.pViewportState = &vpStateInfo;
            pipelineInfo.pRasterizationState = &rasterizationInfo;
            pipelineInfo.pMultisampleState = &multiSampleInfo;
            pipelineInfo.pColorBlendState = &colorBlendInfo;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = m_pipelineLayout;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;
            pipelineInfo.renderPass = nullptr;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        }
        VK_CHECK(vkCreateGraphicsPipelines(*m_pVkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

        InitResource();
    }

    // ================================================================================================================
    HBasicRenderer::~HBasicRenderer()
    {
        for (uint32_t i = 0; i < m_vkResultImgsViews.size(); i++)
        {
            vkDestroyImageView(*m_pVkDevice, m_vkResultImgsViews[i], nullptr);
            vkDestroySampler(*m_pVkDevice, m_vkResultImgsSamplers[i], nullptr);
            vmaDestroyImage(*m_pVmaAllocator, m_vkResultImgs[i], m_vmaResultImgsAllocations[i]);

            vkDestroyImageView(*m_pVkDevice, m_depthImgsViews[i], nullptr);
            vmaDestroyImage(*m_pVmaAllocator, m_depthImgs[i], m_depthImgsAllocations[i]);

            m_pGpuRsrcManager->DestroyGpuResource(m_mvpUboBuffers[i]);
            m_pGpuRsrcManager->DestroyGpuResource(m_lightUboBuffers[i]);
        }

        // Destroy Pipeline
        vkDestroyPipeline(*m_pVkDevice, m_pipeline, nullptr);

        // Destroy the descriptor set layout
        vkDestroyDescriptorSetLayout(*m_pVkDevice, m_descriptorSetLayout, nullptr);

        // Destroy the pipeline layout
        vkDestroyPipelineLayout(*m_pVkDevice, m_pipelineLayout, nullptr);

        // Destroy both of the shader modules
        vkDestroyShaderModule(*m_pVkDevice, m_shaderVertModule, nullptr);
        vkDestroyShaderModule(*m_pVkDevice, m_shaderFragModule, nullptr);
    }

    // ================================================================================================================
    void HBasicRenderer::InitResource()
    {
        VmaAllocationCreateInfo sceneImgsAllocInfo{};
        {
            sceneImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            sceneImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        VmaAllocationCreateInfo depthImgsAllocInfo{};
        {
            depthImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            depthImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        VkExtent3D extent{};
        {
            extent.width = 100;
            extent.height = 100;
            extent.depth = 1;
        }
        
        m_vkResultImgs.resize(m_onFlightResCnt);
        m_vmaResultImgsAllocations.resize(m_onFlightResCnt);
        m_vkResultImgsViews.resize(m_onFlightResCnt);
        m_vkResultImgsSamplers.resize(m_onFlightResCnt);
        m_resultImgsExtents.resize(m_onFlightResCnt);

        m_depthImgs.resize(m_onFlightResCnt);
        m_depthImgsViews.resize(m_onFlightResCnt);
        m_depthImgsAllocations.resize(m_onFlightResCnt);

        m_mvpUboBuffers.resize(m_onFlightResCnt);
        m_lightUboBuffers.resize(m_onFlightResCnt);

        for (uint32_t i = 0; i < m_onFlightResCnt; i++)
        {
            CreateColorDepthImgs(extent, i);

            m_resultImgsExtents[i] = VkExtent2D{ extent.width, extent.height };

            CreateColorDepthImgsViews(i);

            VkSamplerCreateInfo sampler_info{};
            {
                sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler_info.magFilter = VK_FILTER_LINEAR;
                sampler_info.minFilter = VK_FILTER_LINEAR;
                sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
                sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_info.minLod = -1000;
                sampler_info.maxLod = 1000;
                sampler_info.maxAnisotropy = 1.0f;
            }
            VK_CHECK(vkCreateSampler(*m_pVkDevice, &sampler_info, nullptr, &m_vkResultImgsSamplers[i]));

            // Create ubo buffers
            m_mvpUboBuffers[i] = m_pGpuRsrcManager->CreateGpuBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                    32 * sizeof(float));

            m_lightUboBuffers[i] = m_pGpuRsrcManager->CreateGpuBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                      8 * sizeof(float));

            // Write a descriptor to let it point to the mvp matrix buffer.
            VkDescriptorBufferInfo mvpDescriptorBufferInfo{};
            {
                mvpDescriptorBufferInfo.buffer = *m_mvpUboBuffers[i].m_pBuffer;
                mvpDescriptorBufferInfo.offset = 0;
                mvpDescriptorBufferInfo.range = 32 * sizeof(float);
            }

            // Write a descriptor to let it point to the ubo light buffer
            VkDescriptorBufferInfo ptLightDescriptorBufferInfo{};
            {
                ptLightDescriptorBufferInfo.buffer = *m_lightUboBuffers[i].m_pBuffer;
                ptLightDescriptorBufferInfo.offset = 0;
                ptLightDescriptorBufferInfo.range = 8 * sizeof(float);
            }

            VkWriteDescriptorSet mvpDescriptorWrite{};
            {
                mvpDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                mvpDescriptorWrite.dstSet = m_uboDescriptorSets[i];
                mvpDescriptorWrite.dstBinding = 0;
                mvpDescriptorWrite.dstArrayElement = 0;
                mvpDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                mvpDescriptorWrite.descriptorCount = 1;
                mvpDescriptorWrite.pBufferInfo = &mvpDescriptorBufferInfo;
            }

            VkWriteDescriptorSet lightDescriptorWrite{};
            {
                lightDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                lightDescriptorWrite.dstSet = m_uboDescriptorSets[i];
                lightDescriptorWrite.dstBinding = 1;
                lightDescriptorWrite.dstArrayElement = 0;
                lightDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                lightDescriptorWrite.descriptorCount = 1;
                lightDescriptorWrite.pBufferInfo = &ptLightDescriptorBufferInfo;
            }

            VkWriteDescriptorSet desWriteArray[2] = { mvpDescriptorWrite, lightDescriptorWrite };
            vkUpdateDescriptorSets(*m_pVkDevice, 2, desWriteArray, 0, nullptr);
        }
    }

    // ================================================================================================================
    void HBasicRenderer::RecreateResource(
        VkExtent2D resultExtent,
        uint32_t frameIdx)
    {
        vmaDestroyImage(*m_pVmaAllocator, m_vkResultImgs[frameIdx], m_vmaResultImgsAllocations[frameIdx]);
        vmaDestroyImage(*m_pVmaAllocator, m_depthImgs[frameIdx], m_depthImgsAllocations[frameIdx]);

        VkExtent3D extent{};
        {
            extent.width = resultExtent.width;
            extent.height = resultExtent.height;
            extent.depth = 1;
        }
        
        CreateColorDepthImgs(extent, frameIdx);

        m_resultImgsExtents[frameIdx] = resultExtent;

        vkDestroyImageView(*m_pVkDevice, m_vkResultImgsViews[frameIdx], nullptr);
        vkDestroyImageView(*m_pVkDevice, m_depthImgsViews[frameIdx], nullptr);

        CreateColorDepthImgsViews(frameIdx);
    }

    // ================================================================================================================
    bool HBasicRenderer::NeedResize(
        VkExtent2D inExtent,
        uint32_t frameIdx)
    {
        return (inExtent.width != m_resultImgsExtents[frameIdx].width) || 
               (inExtent.height != m_resultImgsExtents[frameIdx].height);
    }

    // ================================================================================================================
    VkImageView* HBasicRenderer::Render(
        VkCommandBuffer& cmdBuf,
        GpuResource idxResource,
        GpuResource vertResource,
        VkExtent2D renderExtent,
        uint32_t   frameIdx,
        const SceneRenderInfo& sceneInfo)
    {
        if (NeedResize(renderExtent, frameIdx))
        {
            RecreateResource(renderExtent, frameIdx);
        }

        // Calculate the mvp matrix
        float vpMatT[16] = {};
        memcpy(vpMatT, sceneInfo.m_vpMat, 16 * sizeof(float));
        MatTranspose(vpMatT, 4);

        float modelT[16] = {};
        memcpy(modelT, sceneInfo.m_modelMat, 16 * sizeof(float));
        MatTranspose(modelT, 4);

        float uboMatBuf[32] = {};
        memcpy(uboMatBuf, modelT, 16 * sizeof(float));
        memcpy(&uboMatBuf[16], vpMatT, 16 * sizeof(float));

        // Transfer mvp matrix data to ubo
        m_pGpuRsrcManager->SendDataToBuffer(m_mvpUboBuffers[frameIdx], uboMatBuf, sizeof(uboMatBuf));

        // Transfer light data to ubo
        float tmpLightData[8] = {
            1.f, 1.f, 1.f, 0.f,
            0.f, 3.f, 0.5f, 0.f
        };
        m_pGpuRsrcManager->SendDataToBuffer(m_lightUboBuffers[frameIdx], tmpLightData, sizeof(tmpLightData));

        // Add a barrier to wait for image format transition and ubo data transfer.
        // Transfer the scene rendering image format from undefined or shader read to shader output.
        VkImageSubresourceRange subResRange{};
        {
            subResRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subResRange.baseMipLevel = 0;
            subResRange.levelCount = 1;
            subResRange.baseArrayLayer = 0;
            subResRange.layerCount = 1;
        }

        VkImageMemoryBarrier sceneImgAsOutputLayoutTransitionBarrier{};
        {
            sceneImgAsOutputLayoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            sceneImgAsOutputLayoutTransitionBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sceneImgAsOutputLayoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            sceneImgAsOutputLayoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            sceneImgAsOutputLayoutTransitionBarrier.image = m_vkResultImgs[frameIdx];
            sceneImgAsOutputLayoutTransitionBarrier.subresourceRange = subResRange;
        }

        // UBO mvp matrix data transfer barrier
        VkMemoryBarrier uboDataTransBarrier{};
        {
            uboDataTransBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            uboDataTransBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            uboDataTransBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        }

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0,
            1, &uboDataTransBarrier,
            0, nullptr,
            0, nullptr);

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 
            0, nullptr, 
            0, nullptr,
            1, &sceneImgAsOutputLayoutTransitionBarrier);    

        // Draw the scene
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

        VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
        {
            colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            colorAttachmentInfo.imageView = m_vkResultImgsViews[frameIdx];
            colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentInfo.clearValue = clearColor;
        }

        VkClearValue depthClearVal{};
        depthClearVal.depthStencil.depth = 0.f;
        VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
        {
            depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            depthAttachmentInfo.imageView = m_depthImgsViews[frameIdx];
            depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachmentInfo.clearValue = depthClearVal;
        }

        VkRenderingInfoKHR renderInfo{};
        {
            renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderInfo.renderArea.offset = { 0, 0 };
            renderInfo.renderArea.extent = renderExtent;
            renderInfo.layerCount = 1;
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachments = &colorAttachmentInfo;
            renderInfo.pDepthAttachment = &depthAttachmentInfo;
        }

        vkCmdBeginRendering(cmdBuf, &renderInfo);

        // Bind the graphics pipeline
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Set the viewport
        VkViewport viewport{};
        {
            viewport.x = 0.f;
            viewport.y = 0.f;
            viewport.width = (float)renderExtent.width;
            viewport.height = (float)renderExtent.height;
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;
        }
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        // Set the scissor
        VkRect2D scissor{};
        {
            scissor.offset = { 0, 0 };
            scissor.extent = renderExtent;
            vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
        }

        // TODO: Make it general enough by using the scene geometries.
        VkDeviceSize vbOffset = 0;
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertResource.m_pBuffer, &vbOffset);
        vkCmdBindIndexBuffer(cmdBuf, *idxResource.m_pBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &m_uboDescriptorSets[frameIdx],
            0,
            nullptr);

        vkCmdDrawIndexed(cmdBuf, sceneInfo.m_vertCnt, 1, 0, 0, 0);

        vkCmdEndRendering(cmdBuf);

        // Transform the layout of the scene image from shader output to shader read for GUI rendering consumption.
        VkImageMemoryBarrier sceneImgLayoutAsInputTransitionBarrier{};
        {
            sceneImgLayoutAsInputTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            sceneImgLayoutAsInputTransitionBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sceneImgLayoutAsInputTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sceneImgLayoutAsInputTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            sceneImgLayoutAsInputTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            sceneImgLayoutAsInputTransitionBarrier.image = m_vkResultImgs[frameIdx];
            sceneImgLayoutAsInputTransitionBarrier.subresourceRange = subResRange;
        }

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &sceneImgLayoutAsInputTransitionBarrier);

        return &m_vkResultImgsViews[frameIdx];
    }

    // ================================================================================================================
    void HBasicRenderer::CreateColorDepthImgs(
        VkExtent3D extent, 
        uint32_t   idx)
    {
        VkImageCreateInfo sceneImgsInfo{};
        {
            sceneImgsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            sceneImgsInfo.imageType = VK_IMAGE_TYPE_2D;
            sceneImgsInfo.format = m_renderSurfFormat;
            sceneImgsInfo.extent = extent;
            sceneImgsInfo.mipLevels = 1;
            sceneImgsInfo.arrayLayers = 1;
            sceneImgsInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            sceneImgsInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            sceneImgsInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            sceneImgsInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkImageCreateInfo depthImgsInfo{};
        {
            depthImgsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImgsInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImgsInfo.format = VK_FORMAT_D16_UNORM;
            depthImgsInfo.extent = extent;
            depthImgsInfo.mipLevels = 1;
            depthImgsInfo.arrayLayers = 1;
            depthImgsInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImgsInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImgsInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthImgsInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VmaAllocationCreateInfo sceneImgsAllocInfo{};
        {
            sceneImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            sceneImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        VmaAllocationCreateInfo depthImgsAllocInfo{};
        {
            depthImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            depthImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        vmaCreateImage(*m_pVmaAllocator,
                       &sceneImgsInfo,
                       &sceneImgsAllocInfo,
                       &m_vkResultImgs[idx],
                       &m_vmaResultImgsAllocations[idx],
                       nullptr);

        vmaCreateImage(*m_pVmaAllocator, 
                       &depthImgsInfo,
                       &depthImgsAllocInfo,
                       &m_depthImgs[idx],
                       &m_depthImgsAllocations[idx],
                       nullptr);
    }

    // ================================================================================================================
    void HBasicRenderer::CreateColorDepthImgsViews(
        uint32_t idx)
    {
        // Create the render result color image View
        VkImageViewCreateInfo colorImgViewInfo = {};
        {
            colorImgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorImgViewInfo.image = m_vkResultImgs[idx];
            colorImgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorImgViewInfo.format = m_renderSurfFormat;
            colorImgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorImgViewInfo.subresourceRange.levelCount = 1;
            colorImgViewInfo.subresourceRange.layerCount = 1;
        }

        VK_CHECK(vkCreateImageView(*m_pVkDevice, &colorImgViewInfo, nullptr, &m_vkResultImgsViews[idx]));

        // Create the depth image view
        VkImageViewCreateInfo depthImgViewInfo = {};
        {
            depthImgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthImgViewInfo.image = m_depthImgs[idx];
            depthImgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthImgViewInfo.format = VK_FORMAT_D16_UNORM;
            depthImgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthImgViewInfo.subresourceRange.levelCount = 1;
            depthImgViewInfo.subresourceRange.layerCount = 1;
        }

        VK_CHECK(vkCreateImageView(*m_pVkDevice, &depthImgViewInfo, nullptr, &m_depthImgsViews[idx]));
    }
}