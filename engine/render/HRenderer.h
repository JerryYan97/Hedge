#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
*/

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "vk_mem_alloc.h"

struct GLFWwindow;

namespace Hedge
{
    struct SceneRenderInfo;
    struct GpuResource;
    class HRenderManager;
    class HGpuRsrcManager;

    class HRenderer
    {
    public:
        explicit HRenderer(uint32_t onFlightResCnt, VkDevice* pVkDevice, VkFormat surfFormat, VmaAllocator* pVmaAllocator);
        virtual ~HRenderer();

        virtual VkImageView* Render(VkCommandBuffer& cmdBuf, 
                                          GpuResource idxResource, 
                                          GpuResource vertResource, 
                                          VkExtent2D renderExtent,
                                          uint32_t frameIdx) = 0;

    protected:
        void CreateShaderModule(VkShaderModule* pShaderModule, uint32_t* pShaderScript, uint32_t scriptByteCnt);

        VkDevice*     m_pVkDevice;
        VkFormat      m_renderSurfFormat;
        VmaAllocator* m_pVmaAllocator;

        const uint32_t m_onFlightResCnt;
        
    private:

    };

    class HBasicRenderer : public HRenderer
    {
    public:
        explicit HBasicRenderer(uint32_t      onFlightResCnt, 
                                VkDevice*     pVkDevice, 
                                VkFormat      surfFormat, 
                                VmaAllocator* pVmaAllocator,
                                HGpuRsrcManager* pGpuRsrcManager);

        virtual ~HBasicRenderer();

        virtual VkImageView* Render(VkCommandBuffer& cmdBuf,
                            GpuResource idxResource, 
                            GpuResource vertResource,
                            VkExtent2D renderExtent,
                            uint32_t   frameIdx) override;

    private:
        void InitResource();
        void RecreateResource(VkExtent2D resultExtent, uint32_t frameIdx);
        inline bool NeedResize(VkExtent2D inExtent, uint32_t frameIdx);

        VkShaderModule        m_shaderVertModule;
        VkShaderModule        m_shaderFragModule;
        VkPipeline            m_pipeline;
        VkPipelineLayout      m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        HGpuRsrcManager*      m_pGpuRsrcManager;

        std::vector<VkImage> m_vkResultImgs;
        std::vector<VkImageView> m_vkResultImgsViews;
        std::vector<VkSampler> m_vkResultImgsSamplers;
        std::vector<VmaAllocation> m_vmaResultImgsAllocations;
        std::vector<VkExtent2D> m_resultImgsExtents;
        std::vector<VkDescriptorSet> m_uboDescriptorSets;
        std::vector<GpuResource> m_uboBuffers;

        uint32_t m_lastFrameIdx;
    };
}
