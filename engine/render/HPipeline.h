#pragma once

#include <vector>
#include <vulkan/vulkan.h>

// The design philosophy of the pipeline is to set the pipeline states or infos along the way and record what we set.
// When we create the pipeline, if we find out that some infos are not fed before, we'll just use the default settings.
//
// In addition, we always use the dynamic rendering.
namespace Hedge
{
    struct HGpuRsrcFrameContext;

    struct PipelineColorBlendInfo
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlending;
    };

    struct PipelineDynamicStatesInfo
    {
        std::vector<VkDynamicState> dynamicStates;
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    };

    class HPipeline
    {
    public:
        HPipeline();
        ~HPipeline();

        VkPipeline GetVkPipeline() { return m_pipeline; }
        VkPipelineLayout GetVkPipelineLayout() { return m_pipelineLayout; }

        void CreatePipeline(VkDevice device);

        void AddShaderStageInfo(VkPipelineShaderStageCreateInfo shaderStgInfo);
        void SetPNext(void* pNext) { m_pNext = pNext; }

        void SetVertexInputInfo(VkPipelineVertexInputStateCreateInfo* pVertexInputInfo)
        {
            m_pVertexInputInfo = pVertexInputInfo;
        }

        void SetPipelineLayout(VkPipelineLayout pipelineLayout) { m_pipelineLayout = pipelineLayout; }
        void SetDepthStencilStateInfo(VkPipelineDepthStencilStateCreateInfo* pDepthStencilInfo)
        {
            m_pDepthStencilState = pDepthStencilInfo;
        }

        virtual void CmdBindDescriptors(VkCommandBuffer cmdBuf, const HGpuRsrcFrameContext* const pGpuRsrcCtx) = 0;

    protected:
        VkPipelineShaderStageCreateInfo CreateDefaultShaderStgCreateInfo(const VkShaderModule& shaderModule, const VkShaderStageFlagBits stg);
        VkShaderModule CreateShaderModule(const uint32_t* pShaderScript, uint32_t bytesCnt);

        virtual void CreateSetCustomPipelineInfo() = 0;

        void AddDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout) { m_descriptorSetLayouts.push_back(descriptorSetLayout); }

        void CleanupHeapMemory();

        std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
        std::vector<VkShaderModule>        m_shaderModules;
        VkDevice                           m_device;
        std::vector<void*>                 m_heapMem;
        std::vector<void*>                 m_heapArrayMem;

    private:
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStgInfos;

        void* m_pNext;

        VkPipelineVertexInputStateCreateInfo* m_pVertexInputInfo;
        VkPipelineInputAssemblyStateCreateInfo* m_pInputAssembly;
        VkPipelineViewportStateCreateInfo* m_pViewportState;
        VkPipelineRasterizationStateCreateInfo* m_pRasterizer;
        VkPipelineMultisampleStateCreateInfo* m_pMultisampling;
        PipelineColorBlendInfo* m_pColorBlending;
        PipelineDynamicStatesInfo* m_pDynamicState;
        VkPipelineDepthStencilStateCreateInfo* m_pDepthStencilState;

        bool m_isVertexInputInfoDefault;

        void SetDefaultVertexInputInfo();
        void SetDefaultInputAssemblyInfo();
        void SetDefaultViewportStateInfo();
        void SetDefaultRasterizerInfo();
        void SetDefaultMultisamplingInfo();
        void SetDefaultColorBlendingInfo();
        void SetDefaultDynamicStateInfo();

        VkPipeline       m_pipeline;
        VkPipelineLayout m_pipelineLayout;
    };

    // The PBR pipeline is a fixed pipeline that only uses pbr_vertScript and pbr_fragScript in the g_prebuiltShaders.h.
    class PBRPipeline : public HPipeline
    {
    public:
        PBRPipeline();
        ~PBRPipeline();

        virtual void CmdBindDescriptors(VkCommandBuffer cmdBuf, const HGpuRsrcFrameContext* const pGpuRsrcCtx);

    protected:
        virtual void CreateSetCustomPipelineInfo() override;

    private:
        void CreateSetDescriptorSetLayouts();
        void CreateSetPBRPipelineLayout();
        VkPipelineVertexInputStateCreateInfo CreatePipelineVertexInputInfo();
        VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo();

        static const VkFormat m_colorAttachmentFormat = VK_FORMAT_R8G8B8A8_SRGB;
    };
}