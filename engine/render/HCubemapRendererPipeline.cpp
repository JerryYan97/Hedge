#include "HCubemapRendererPipeline.h"
#include "Utils.h"
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

    }
} // End of namespace Hedge.