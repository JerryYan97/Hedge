#include "HPipeline.h"
#include "Utils.h"
#include <cassert>
#include "../render/HRenderManager.h"
#include "g_prebuiltShaders.h"

namespace Hedge
{
    // ================================================================================================================
    HPipeline::HPipeline() :
        m_pipeline(VK_NULL_HANDLE),
        m_pNext(nullptr),
        m_pVertexInputInfo(nullptr),
        m_pInputAssembly(nullptr),
        m_pViewportState(nullptr),
        m_pRasterizer(nullptr),
        m_pMultisampling(nullptr),
        m_pColorBlending(nullptr),
        m_pDynamicState(nullptr),
        m_pipelineLayout(VK_NULL_HANDLE),
        m_isVertexInputInfoDefault(false),
        m_device(VK_NULL_HANDLE),
        m_pDepthStencilState(nullptr),
        m_pfnCmdPushDescriptorSet(VK_NULL_HANDLE)
    {}

    // ================================================================================================================
    HPipeline::~HPipeline()
    {
        if ((m_pVertexInputInfo != nullptr) && m_isVertexInputInfoDefault)
        {
            delete m_pVertexInputInfo;
        }

        if (m_pInputAssembly != nullptr)
        {
            delete m_pInputAssembly;
        }

        if (m_pViewportState != nullptr)
        {
            delete m_pViewportState;
        }

        if (m_pRasterizer != nullptr)
        {
            delete m_pRasterizer;
        }

        if (m_pMultisampling != nullptr)
        {
            delete m_pMultisampling;
        }

        if (m_pColorBlending != nullptr)
        {
            delete m_pColorBlending;
        }

        if (m_pDynamicState != nullptr)
        {
            delete m_pDynamicState;
        }

        // Destroy the pipeline
        if (m_device != VK_NULL_HANDLE)
        {
            for (VkShaderModule shaderModule : m_shaderModules)
            {
                vkDestroyShaderModule(m_device, shaderModule, nullptr);
            }

            for (VkDescriptorSetLayout descriptorSetLayout : m_descriptorSetLayouts)
            {
                vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);
            }

            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultVertexInputInfo()
    {
        m_pVertexInputInfo = new VkPipelineVertexInputStateCreateInfo();
        memset(m_pVertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
        m_isVertexInputInfoDefault = true;
        {
            m_pVertexInputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            m_pVertexInputInfo->vertexBindingDescriptionCount = 0;
            m_pVertexInputInfo->vertexAttributeDescriptionCount = 0;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultInputAssemblyInfo()
    {
        m_pInputAssembly = new VkPipelineInputAssemblyStateCreateInfo();
        memset(m_pInputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
        {
            m_pInputAssembly->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            m_pInputAssembly->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            m_pInputAssembly->primitiveRestartEnable = VK_FALSE;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultViewportStateInfo()
    {
        m_pViewportState = new VkPipelineViewportStateCreateInfo();
        memset(m_pViewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));
        {
            m_pViewportState->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            m_pViewportState->viewportCount = 1;
            m_pViewportState->scissorCount = 1;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultRasterizerInfo()
    {
        m_pRasterizer = new VkPipelineRasterizationStateCreateInfo();
        memset(m_pRasterizer, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
        {
            m_pRasterizer->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            m_pRasterizer->depthClampEnable = VK_FALSE;
            m_pRasterizer->rasterizerDiscardEnable = VK_FALSE;
            m_pRasterizer->polygonMode = VK_POLYGON_MODE_FILL;
            m_pRasterizer->lineWidth = 1.0f;
            // m_pRasterizer->cullMode = VK_CULL_MODE_BACK_BIT;
            m_pRasterizer->cullMode = VK_CULL_MODE_NONE;
            // NOTE: We reverse the depth so the faces' winding is also reversed.
            // m_pRasterizer->frontFace = VK_FRONT_FACE_CLOCKWISE;
            m_pRasterizer->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            m_pRasterizer->depthBiasEnable = VK_FALSE;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultMultisamplingInfo()
    {
        m_pMultisampling = new VkPipelineMultisampleStateCreateInfo();
        memset(m_pMultisampling, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
        {
            m_pMultisampling->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            m_pMultisampling->sampleShadingEnable = VK_FALSE;
            m_pMultisampling->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultColorBlendingInfo()
    {
        m_pColorBlending = new PipelineColorBlendInfo();
        memset(m_pColorBlending, 0, sizeof(PipelineColorBlendInfo));
        {
            m_pColorBlending->colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            m_pColorBlending->colorBlendAttachment.blendEnable = VK_FALSE;

            m_pColorBlending->colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            m_pColorBlending->colorBlending.logicOpEnable = VK_FALSE;
            m_pColorBlending->colorBlending.logicOp = VK_LOGIC_OP_COPY;
            m_pColorBlending->colorBlending.attachmentCount = 1;
            m_pColorBlending->colorBlending.pAttachments = &(m_pColorBlending->colorBlendAttachment);
            m_pColorBlending->colorBlending.blendConstants[0] = 0.0f;
            m_pColorBlending->colorBlending.blendConstants[1] = 0.0f;
            m_pColorBlending->colorBlending.blendConstants[2] = 0.0f;
            m_pColorBlending->colorBlending.blendConstants[3] = 0.0f;
        }
    }

    // ================================================================================================================
    void HPipeline::SetDefaultDynamicStateInfo()
    {
        m_pDynamicState = new PipelineDynamicStatesInfo();
        memset(m_pDynamicState, 0, sizeof(PipelineDynamicStatesInfo));
        {
            m_pDynamicState->dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            m_pDynamicState->dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

            m_pDynamicState->dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            m_pDynamicState->dynamicStateCreateInfo.dynamicStateCount =
                static_cast<uint32_t>(m_pDynamicState->dynamicStates.size());
            m_pDynamicState->dynamicStateCreateInfo.pDynamicStates = m_pDynamicState->dynamicStates.data();
        }
    }

    // ================================================================================================================
    void HPipeline::CreatePipeline(
        VkDevice device)
    {
        m_device = device;

        CreateSetCustomPipelineInfo();

        if (m_pVertexInputInfo == nullptr)
        {
            SetDefaultVertexInputInfo();
        }

        if (m_pInputAssembly == nullptr)
        {
            SetDefaultInputAssemblyInfo();
        }

        if (m_pViewportState == nullptr)
        {
            SetDefaultViewportStateInfo();
        }

        if (m_pRasterizer == nullptr)
        {
            SetDefaultRasterizerInfo();
        }

        if (m_pMultisampling == nullptr)
        {
            SetDefaultMultisamplingInfo();
        }

        if (m_pColorBlending == nullptr)
        {
            SetDefaultColorBlendingInfo();
        }

        if (m_pDynamicState == nullptr)
        {
            SetDefaultDynamicStateInfo();
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        {
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.pNext = m_pNext;
            pipelineInfo.stageCount = m_shaderStgInfos.size();
            pipelineInfo.pStages = m_shaderStgInfos.data();
            pipelineInfo.pVertexInputState = m_pVertexInputInfo;
            pipelineInfo.pInputAssemblyState = m_pInputAssembly;
            pipelineInfo.pViewportState = m_pViewportState;
            pipelineInfo.pRasterizationState = m_pRasterizer;
            pipelineInfo.pMultisampleState = m_pMultisampling;
            pipelineInfo.pColorBlendState = &(m_pColorBlending->colorBlending);
            pipelineInfo.pDynamicState = &(m_pDynamicState->dynamicStateCreateInfo);
            pipelineInfo.layout = m_pipelineLayout;
            pipelineInfo.renderPass = nullptr;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.pDepthStencilState = m_pDepthStencilState;
        }

        VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

        m_pfnCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(m_device,
                                                                                       "vkCmdPushDescriptorSetKHR");
        if (!m_pfnCmdPushDescriptorSet) {
            exit(1);
        }
    }

    // ================================================================================================================
    void HPipeline::AddShaderStageInfo(
        VkPipelineShaderStageCreateInfo shaderStgInfo)
    {
        m_shaderStgInfos.push_back(shaderStgInfo);
    }

    // ================================================================================================================
    VkShaderModule HPipeline::CreateShaderModule(
        const uint32_t* pShaderScript,
        uint32_t        bytesCnt)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        {
            shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCreateInfo.codeSize = bytesCnt;
            shaderModuleCreateInfo.pCode = pShaderScript;
        }
        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &shaderModule));

        return shaderModule;
    }

    // ================================================================================================================
    VkPipelineShaderStageCreateInfo HPipeline::CreateDefaultShaderStgCreateInfo(
        const VkShaderModule& shaderModule,
        const VkShaderStageFlagBits stg)
    {
        VkPipelineShaderStageCreateInfo info{};
        {
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.stage = stg;
            info.module = shaderModule;
            info.pName = "main";
            info.pSpecializationInfo = nullptr;
        }
        return info;
    }

    // ================================================================================================================
    void HPipeline::CleanupHeapMemory()
    {
        for (void* pData : m_heapMem)
        {
            delete pData;
        }

        for (void* pArrayData : m_heapArrayMem)
        {
            delete[] pArrayData;
        }
    }

    // ================================================================================================================
    PBRPipeline::PBRPipeline() :
        HPipeline()
    {

    }

    // ================================================================================================================
    PBRPipeline::~PBRPipeline()
    {

    }

    // ================================================================================================================
    void PBRPipeline::CreateSetDescriptorSetLayouts()
    {
        // Create pipeline's descriptors layout
        // The Vulkan spec states: The VkDescriptorSetLayoutBinding::binding members of the elements of the pBindings array 
        // must each have different values 
        // (https://vulkan.lunarg.com/doc/view/1.3.236.0/windows/1.3-extensions/vkspec.html#VUID-VkDescriptorSetLayoutCreateInfo-binding-00279)

        // Create pipeline binding and descriptor objects for the camera parameters
        std::vector<VkDescriptorSetLayoutBinding> pbrDescriptorSetBindings;

        // Binding related to the scene info ubo
        VkDescriptorSetLayoutBinding vpMatUboBinding{};
        {
            vpMatUboBinding.binding = 0;
            vpMatUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            vpMatUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            vpMatUboBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(vpMatUboBinding);

        // Bindings related to the IBL
        VkDescriptorSetLayoutBinding diffuseIrradianceBinding{};
        {
            diffuseIrradianceBinding.binding = 1;
            diffuseIrradianceBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            diffuseIrradianceBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            diffuseIrradianceBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(diffuseIrradianceBinding);

        VkDescriptorSetLayoutBinding prefilterEnvBinding{};
        {
            prefilterEnvBinding.binding = 2;
            prefilterEnvBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            prefilterEnvBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            prefilterEnvBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(prefilterEnvBinding);

        VkDescriptorSetLayoutBinding envBrdfBinding{};
        {
            envBrdfBinding.binding = 3;
            envBrdfBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            envBrdfBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            envBrdfBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(envBrdfBinding);

        // Bindings related to object's material
        VkDescriptorSetLayoutBinding baseColorBinding{};
        {
            baseColorBinding.binding = 4;
            baseColorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            baseColorBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            baseColorBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(baseColorBinding);

        VkDescriptorSetLayoutBinding normalBinding{};
        {
            normalBinding.binding = 5;
            normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normalBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(normalBinding);

        VkDescriptorSetLayoutBinding metallicRoughnessBinding{};
        {
            metallicRoughnessBinding.binding = 6;
            metallicRoughnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            metallicRoughnessBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            metallicRoughnessBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(metallicRoughnessBinding);

        VkDescriptorSetLayoutBinding occlusionBinding{};
        {
            occlusionBinding.binding = 7;
            occlusionBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            occlusionBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            occlusionBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(occlusionBinding);

        // Bindings related to point lights. We don't know how many lights in the scene at a specific frame, so we need
        // to use the storage buffer.
        VkDescriptorSetLayoutBinding ptLightsPosBinding{};
        {
            ptLightsPosBinding.binding = 8;
            ptLightsPosBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            ptLightsPosBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            ptLightsPosBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(ptLightsPosBinding);

        VkDescriptorSetLayoutBinding ptLightRadianceBinding{};
        {
            ptLightRadianceBinding.binding = 9;
            ptLightRadianceBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            ptLightRadianceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            ptLightRadianceBinding.descriptorCount = 1;
        }
        pbrDescriptorSetBindings.push_back(ptLightRadianceBinding);

        // Create descriptor layouts create infos
        VkDescriptorSetLayoutCreateInfo pbrDesSetLayoutInfo{};
        {
            pbrDesSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            pbrDesSetLayoutInfo.bindingCount = pbrDescriptorSetBindings.size();
            pbrDesSetLayoutInfo.pBindings = pbrDescriptorSetBindings.data();
        }

        // Create the descriptor set layout
        VkDescriptorSetLayout pbrDescriptorSetLayout;
        VK_CHECK(vkCreateDescriptorSetLayout(m_device,
                                             &pbrDesSetLayoutInfo,
                                             nullptr,
                                             &pbrDescriptorSetLayout));

        AddDescriptorSetLayout(pbrDescriptorSetLayout);
    }

    // ================================================================================================================
    void PBRPipeline::CreateSetPBRPipelineLayout()
    {
        VkPushConstantRange range = {};
        {
            range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            range.offset = 0;
            range.size = 4 * sizeof(float) + sizeof(uint32_t); // Camera pos, Max IBL mipmap and lights count.
        }

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = m_descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &range;
        }

        VkPipelineLayout pbrPipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pbrPipelineLayout));
        SetPipelineLayout(pbrPipelineLayout);
    }

    // ================================================================================================================
    void PBRPipeline::CreateSetCustomPipelineInfo()
    {
        VkPipelineRenderingCreateInfoKHR pipelineRenderCreateInfo{};
        {
            pipelineRenderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderCreateInfo.colorAttachmentCount = 1;
            pipelineRenderCreateInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
            pipelineRenderCreateInfo.depthAttachmentFormat = VK_FORMAT_D16_UNORM;
        }

        VkPipelineRenderingCreateInfoKHR* pPipelineRenderCreateInfo = new VkPipelineRenderingCreateInfoKHR();
        memcpy(pPipelineRenderCreateInfo, &pipelineRenderCreateInfo, sizeof(VkPipelineRenderingCreateInfoKHR));
        SetPNext(pPipelineRenderCreateInfo);

        // Load shader scripts and create shader modules
        VkShaderModule vertShaderModule = CreateShaderModule((uint32_t*)pbr_vertScript, sizeof(pbr_vertScript));
        VkShaderModule fragShaderModule = CreateShaderModule((uint32_t*)pbr_fragScript, sizeof(pbr_fragScript));
        
        AddShaderStageInfo(CreateDefaultShaderStgCreateInfo(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT));
        AddShaderStageInfo(CreateDefaultShaderStgCreateInfo(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT));

        m_shaderModules.push_back(vertShaderModule);
        m_shaderModules.push_back(fragShaderModule);

        // Create descriptor set layouts
        CreateSetDescriptorSetLayouts();
        
        // Create pipeline layout
        CreateSetPBRPipelineLayout();

        VkPipelineVertexInputStateCreateInfo vertInputInfo = CreatePipelineVertexInputInfo();
        VkPipelineVertexInputStateCreateInfo* pVertInputInfo = new VkPipelineVertexInputStateCreateInfo();
        memcpy(pVertInputInfo, &vertInputInfo, sizeof(vertInputInfo));
        SetVertexInputInfo(pVertInputInfo);
        m_heapMem.push_back(pVertInputInfo);

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = CreateDepthStencilStateInfo();
        VkPipelineDepthStencilStateCreateInfo* pDepthStencilInfo = new VkPipelineDepthStencilStateCreateInfo();
        memcpy(pDepthStencilInfo, &depthStencilInfo, sizeof(depthStencilInfo));
        SetDepthStencilStateInfo(pDepthStencilInfo);
        m_heapMem.push_back(pDepthStencilInfo);
    }

    // ================================================================================================================
    VkPipelineVertexInputStateCreateInfo PBRPipeline::CreatePipelineVertexInputInfo()
    {
        // Specifying all kinds of pipeline states
        // Vertex input state
        VkVertexInputBindingDescription* pVertBindingDesc = new VkVertexInputBindingDescription();
        memset(pVertBindingDesc, 0, sizeof(VkVertexInputBindingDescription));
        {
            pVertBindingDesc->binding = 0;
            pVertBindingDesc->stride = 12 * sizeof(float);
            pVertBindingDesc->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }
        m_heapMem.push_back(pVertBindingDesc);

        VkVertexInputAttributeDescription* pVertAttrDescs = new VkVertexInputAttributeDescription[4];
        memset(pVertAttrDescs, 0, sizeof(VkVertexInputAttributeDescription) * 4);
        {
            // Position
            pVertAttrDescs[0].location = 0;
            pVertAttrDescs[0].binding = 0;
            pVertAttrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            pVertAttrDescs[0].offset = 0;
            // Normal
            pVertAttrDescs[1].location = 1;
            pVertAttrDescs[1].binding = 0;
            pVertAttrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            pVertAttrDescs[1].offset = 3 * sizeof(float);
            // Tangent
            pVertAttrDescs[2].location = 2;
            pVertAttrDescs[2].binding = 0;
            pVertAttrDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            pVertAttrDescs[2].offset = 6 * sizeof(float);
            // Texcoord
            pVertAttrDescs[3].location = 3;
            pVertAttrDescs[3].binding = 0;
            pVertAttrDescs[3].format = VK_FORMAT_R32G32_SFLOAT;
            pVertAttrDescs[3].offset = 10 * sizeof(float);
        }
        m_heapArrayMem.push_back(pVertAttrDescs);

        VkPipelineVertexInputStateCreateInfo vertInputInfo{};
        {
            vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertInputInfo.pNext = nullptr;
            vertInputInfo.vertexBindingDescriptionCount = 1;
            vertInputInfo.pVertexBindingDescriptions = pVertBindingDesc;
            vertInputInfo.vertexAttributeDescriptionCount = 4;
            vertInputInfo.pVertexAttributeDescriptions = pVertAttrDescs;
        }

        return vertInputInfo;
    }

    // ================================================================================================================
    VkPipelineDepthStencilStateCreateInfo PBRPipeline::CreateDepthStencilStateInfo()
    {
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        {
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilInfo.depthTestEnable = VK_TRUE;
            depthStencilInfo.depthWriteEnable = VK_TRUE;
            depthStencilInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // Reverse depth for higher precision. 
            depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            depthStencilInfo.stencilTestEnable = VK_FALSE;
        }

        return depthStencilInfo;
    }

    // ================================================================================================================
    void HPipeline::CmdBindDescriptors(
        VkCommandBuffer                        cmdBuf,
        const std::vector<ShaderInputBinding>& bindings)
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSetsVec;
        
        for (uint32_t i = 0; i < bindings.size(); i++)
        {
            ShaderInputBinding binding = bindings[i];
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstBinding = i;
            writeDescriptorSet.descriptorCount = 1;

            if (binding.first == HGPU_BUFFER)
            {
                HGpuBuffer* pGpuBuffer = static_cast<HGpuBuffer*>(binding.second);
                writeDescriptorSet.descriptorType = pGpuBuffer->gpuBufferDescriptorType;
                writeDescriptorSet.pBufferInfo = &pGpuBuffer->gpuBufferDescriptorInfo;
            }
            else
            {
                HGpuImg* pGpuImg = static_cast<HGpuImg*>(binding.second);
                writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSet.pImageInfo = &pGpuImg->gpuImgDescriptorInfo;
            }

            writeDescriptorSetsVec.push_back(writeDescriptorSet);
        }

        m_pfnCmdPushDescriptorSet(cmdBuf,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  m_pipelineLayout,
                                  0, writeDescriptorSetsVec.size(), writeDescriptorSetsVec.data());
    }
}