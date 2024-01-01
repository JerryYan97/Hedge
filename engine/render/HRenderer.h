#pragma once

/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderers.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering. It has the knowledge and manage specific pipelines.
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
// TODO9: Maybe I want to shield users from vulkan details for inheritance class as much as possible?
// TODO10: Pipeline is controlled by the render manager. Pipeline is aware of the frames. Pipeline should control the descriptor sets.
// TODO11: Renderer is more or less a boilerplate. It gets pipeline and all nesseary information and draw to the designated image. The base renderer should consider the case that a render needs mutiple pipelines.

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "HPipeline.h"

namespace Hedge
{
    class HPipeline;
    struct HGpuBuffer;

    struct HRenderContext
    {
        HGpuBuffer* pIdxBuffer;
        HGpuBuffer* pVertBuffer;

        std::vector<ShaderInputBinding> bindings;
        // HGpuBuffer uboBuffer;
        // std::vector<VkDescriptorSet> descriptorSets;
        
        uint32_t idxCnt;

        // Assume that both of them are under the VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR.
        // VkImageView colorAttachmentImgView;
        // VkImageView depthAttachmentImgView;
        HGpuImg* pColorAttachmentImg;
        HGpuImg* pDepthAttachmentImg;
        VkRect2D renderArea;
        
        void*    pPushConstantData;
        uint32_t pushConstantDataBytesCnt;
    };

    class HRenderer
    {
    public:
        explicit HRenderer(VkDevice device);
        virtual ~HRenderer();

        virtual void CmdRenderInsts(VkCommandBuffer& cmdBuf, const HRenderContext* const pRenderCtx) = 0;
        void CmdTransImgLayout(VkCommandBuffer& cmdBuf, const HGpuImg* const pGpuImg, VkImageLayout targetLayout);

        void AddPipeline(HPipeline* pPipeline) { m_pPipelines.push_back(pPipeline); };

    protected:
        std::vector<HPipeline*> m_pPipelines;
        VkDevice                m_device;

    private:
    };

    // A basic one pipeline renderer.
    // TODO: Currently, the basic renderer is specific to the PBR pipeline.
    //       In the future, we may want to make it a parent class so that PBR pipeline, cubemap rendering pipeline
    //       IBL generation pipeline can derive from it.
    class HBasicRenderer : public HRenderer
    {
    public:
        explicit HBasicRenderer(VkDevice device);

        virtual ~HBasicRenderer();

        virtual void CmdRenderInsts(VkCommandBuffer& cmdBuf, const HRenderContext* const pRenderCtx) override;
        
    private:
    };
}
