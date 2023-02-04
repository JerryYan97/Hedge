#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
*/

#include <vulkan/vulkan.h>
#include <iostream>

struct GLFWwindow;
class VmaAllocator;
class VmaAllocation;

namespace Hedge
{
    struct SceneRenderInfo;
    struct GpuResource;
    class HRenderManager;

    class HRenderer
    {
    public:
        explicit HRenderer(uint32_t onFlightResCnt, VkDevice* pVkDevice, VkFormat surfFormat, VmaAllocator* pVmaAllocator);
        ~HRenderer();

        virtual void Render(VkCommandBuffer& cmdBuf, GpuResource idxResource, GpuResource vertResource) = 0;

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
        explicit HBasicRenderer(uint32_t onFlightResCnt, VkDevice* pVkDevice, VkFormat surfFormat, VmaAllocator* pVmaAllocator);
        ~HBasicRenderer();

        virtual void Render(VkCommandBuffer& cmdBuf, GpuResource idxResource, GpuResource vertResource) override;

    private:
        void InitResource();
        void RecreateResource();

        VkShaderModule m_shaderVertModule;
        VkShaderModule m_shaderFragModule;
        VkPipeline     m_pipeline;

        std::vector<VkImage> m_vkImgs;
        std::vector<VmaAllocation> m_vmaAllocations;
    };
}
