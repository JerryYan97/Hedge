#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
* A renderer also holds a pipeline.
* What should be a renderer input and output? It's output must be an image. Should the renderer manages the output image? Not really.
* Should a Renderer be aware of the swapchain? It doesn't make to much sense to me since a renderer should also be able to dump just one image or a cubemap.
*/

// Q1: Should I have a more flexible pipeline? Create and modify between frames?

// TODO1: Pipeline should use HPipeline instead of the vkPipeline directly
// TODO2: Render function should be renamed. It should be something like CmdRenderInsts(...).
// TODO3: GpuResources should be renamed to HGpuBuffer. Add a HGpuImg struct. They all should be holded (by using a hashtable, controlling init and destroy, use a heap) by HGpuRsrcManager instead of being managed by Render or other things.
// TODO4: Renderer should be objects with same materials rendering + scene light information instead of the knowing information of the whole scene.
// TODO5: Renderer vert buffer and idx buffers should be managed by a frame rendering context to tmp store rendering information, because objects can be deleted between frames.
// TODO6: Materials and renderers should have connection. A type of materials is only renderable by a renderer.
// TODO7: A renderer should hold and manage one or more pipelines.
// TODO8: The viewport should be the Window size's [max, max], or the shape of the rendered result will be strached. It also means that I need a staging image... Ok, what if I need staging images and buffers?
// The render manager shouldn't be aware of staging images and buffers since it isn't fleasible and it's too customized (It can be general). Maybe the render manager should have an API to manage such frame related rendering rsrcs?
// Ok... Then the renderer cannot directly use the GpuRsrcManager. It has to access the GpuRsrcManager through the render manager so the render manager can track all these resources and release them at the right time.
// Maybe have a HGpuRenderRsrcControl class. So, we don't need to directly access GpuRsrcManager. The controller is a member variable of the manager. It controls UBO and lots of other things.
// Vert in NDC -- <viewport transformation> --> Verts in Framebuffer-Space or Framebuffer Coordinates.

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "vk_mem_alloc.h"

struct GLFWwindow;

namespace Hedge
{
    struct SceneRenderInfo;
    struct HGpuBuffer;
    class HRenderManager;
    class HGpuRsrcManager;
    class HPipeline;

    class HRenderer
    {
    public:
        explicit HRenderer(VkDevice* pVkDevice, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HRenderer();

        virtual void CmdRenderInsts(VkCommandBuffer& cmdBuf) = 0;

    protected:
        VkDevice*               m_pVkDevice;
        HGpuRsrcManager*        m_pGpuRsrcManager;
        std::vector<HPipeline*> m_pPipelines;
    private:

    };

    // A lambertain renderer that uses idx buffer and vert buffer.
    class HBasicRenderer : public HRenderer
    {
    public:
        explicit HBasicRenderer(uint32_t      onFlightResCnt, 
                                VkDevice*     pVkDevice, 
                                VkFormat      surfFormat, 
                                VmaAllocator* pVmaAllocator,
                                HGpuRsrcManager* pGpuRsrcManager);

        virtual ~HBasicRenderer();

        virtual void CmdRenderInsts(VkCommandBuffer& cmdBuf) override;

    private:
        void InitResource();
        void RecreateResource(VkExtent2D resultExtent, uint32_t frameIdx);
        inline bool NeedResize(VkExtent2D inExtent, uint32_t frameIdx);

        void CreateColorDepthImgs(VkExtent3D extent, uint32_t idx);
        void CreateColorDepthImgsViews(uint32_t idx);

        VkShaderModule        m_shaderVertModule;
        VkShaderModule        m_shaderFragModule;
        VkPipeline            m_pipeline;
        VkPipelineLayout      m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        HGpuRsrcManager*      m_pGpuRsrcManager;

        // Color attachment
        std::vector<VkImage> m_vkResultImgs;
        std::vector<VkImageView> m_vkResultImgsViews;
        std::vector<VkSampler> m_vkResultImgsSamplers;
        std::vector<VmaAllocation> m_vmaResultImgsAllocations;
        std::vector<VkExtent2D> m_resultImgsExtents;

        // Depth attachment
        std::vector<VkImage> m_depthImgs;
        std::vector<VkImageView> m_depthImgsViews;
        std::vector<VmaAllocation> m_depthImgsAllocations;
        
        // Shader UBO
        std::vector<VkDescriptorSet> m_uboDescriptorSets;
        std::vector<GpuResource> m_mvpUboBuffers;
        std::vector<GpuResource> m_lightUboBuffers;

        uint32_t m_lastFrameIdx;
    };
}
