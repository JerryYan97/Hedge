#include "HRenderer.h"
#include "HRenderManager.h"
#include "../logging/HLogger.h"
#include "../scene/HScene.h"
#include "Utils.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <vector>
#include <set>

// TODO: Built-in shader generate header. E.g. Python script to generate g_shaderScript.h to replace the one in Util.h.

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
        VmaAllocator* pVmaAllocator)
        : HRenderer(onFlightResCnt, pVkDevice, surfFormat, pVmaAllocator)
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
            vertBindingDesc.stride = 6 * sizeof(float);
            vertBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        VkVertexInputAttributeDescription vertAttrDesc[2];
        {
            // Position
            vertAttrDesc[0].location = 0;
            vertAttrDesc[0].binding = 0;
            vertAttrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertAttrDesc[0].offset = 0;
            // Color
            vertAttrDesc[1].location = 1;
            vertAttrDesc[1].binding = 0;
            vertAttrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertAttrDesc[1].offset = 3 * sizeof(float);
        }
        VkPipelineVertexInputStateCreateInfo vertInputInfo{};
        {
            vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertInputInfo.pNext = nullptr;
            vertInputInfo.vertexBindingDescriptionCount = 1;
            vertInputInfo.pVertexBindingDescriptions = &vertBindingDesc;
            vertInputInfo.vertexAttributeDescriptionCount = 2;
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
            depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
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
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Create a pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0;
        }
        VkPipelineLayout pipelineLayout{};
        VK_CHECK(vkCreatePipelineLayout(*m_pVkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // Create the graphics pipeline -- The graphics pipeline is used for scene rendering
        VkPipelineRenderingCreateInfoKHR pipelineRenderCreateInfo{};
        {
            pipelineRenderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderCreateInfo.colorAttachmentCount = 1;
            pipelineRenderCreateInfo.pColorAttachmentFormats = &surfFormat;
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
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;
            pipelineInfo.renderPass = nullptr;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        }
        VK_CHECK(vkCreateGraphicsPipelines(*m_pVkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

        InitResource();
    }

    // ================================================================================================================
    HBasicRenderer::~HBasicRenderer()
    {}

    // ================================================================================================================
    void HBasicRenderer::InitResource()
    {
        VmaAllocationCreateInfo sceneImgsAllocInfo{};
        {
            sceneImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            sceneImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        VkExtent3D extent{};
        {
            extent.width = 100;
            extent.height = 100;
            extent.depth = 1;
        }

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
        
        m_vkResultImgs.resize(m_onFlightResCnt);
        m_vmaResultImgsAllocations.resize(m_onFlightResCnt);

        for (uint32_t i = 0; i < m_onFlightResCnt; i++)
        {
            vmaCreateImage(*m_pVmaAllocator,
                           &sceneImgsInfo,
                &sceneImgsAllocInfo,
                &m_vkResultImgs[i],
                &m_vmaResultImgsAllocations[i],
                nullptr);

            m_resultImgsExtents[i] = VkExtent2D{ extent.width, extent.height };

            // Create the Image View
            VkImageViewCreateInfo info = {};
            {
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.image = m_vkResultImgs[i];
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = m_renderSurfFormat;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.layerCount = 1;
            }

            VK_CHECK(vkCreateImageView(*m_pVkDevice, &info, nullptr, &m_vkResultImgsViews[i]));

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
        }
    }

    // ================================================================================================================
    void HBasicRenderer::RecreateResource(
        VkExtent2D resultExtent,
        uint32_t frameIdx)
    {
        vmaDestroyImage(*m_pVmaAllocator, m_vkResultImgs[frameIdx], m_vmaResultImgsAllocations[frameIdx]);

        VmaAllocationCreateInfo sceneImgsAllocInfo{};
        {
            sceneImgsAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            sceneImgsAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        VkExtent3D extent{};
        {
            extent.width = resultExtent.width;
            extent.height = resultExtent.height;
            extent.depth = 1;
        }

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
            sceneImgsInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            sceneImgsInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        vmaCreateImage(*m_pVmaAllocator,
            &sceneImgsInfo,
            &sceneImgsAllocInfo,
            &m_vkResultImgs[frameIdx],
            &m_vmaResultImgsAllocations[frameIdx],
            nullptr);

        m_resultImgsExtents[frameIdx] = resultExtent;

        vkDestroyImageView(*m_pVkDevice, m_vkResultImgsViews[frameIdx], nullptr);

        // Create the Image View
        VkImageViewCreateInfo info = {};
        {
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = m_vkResultImgs[frameIdx];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = m_renderSurfFormat;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
        }
        VK_CHECK(vkCreateImageView(*m_pVkDevice, &info, nullptr, &m_vkResultImgsViews[frameIdx]));
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
    void HBasicRenderer::Render(
        VkCommandBuffer& cmdBuf,
        GpuResource idxResource, 
        GpuResource vertResource,
        VkExtent2D renderExtent,
        uint32_t   frameIdx)
    {
        if (NeedResize(renderExtent, frameIdx))
        {
            RecreateResource(renderExtent, frameIdx);
        }


    }
}