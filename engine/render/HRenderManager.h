#pragma once
/*
* The hedge render manager manages the gpu context for each frames, glfw window context, swapchain and a set of renderers.
* The manager also manages the sync between renderers for the final rendering result on the screen.
* The render manager is responsible for everything that can be rendered on the screen, which also includes window's frame.
* 
* A renderer is an entity to construct a command buffer or a set of command buffers for a specific rendering result.
* The specific rendering result would be collected by the render manager to build the final rendering result.
*/

#include <vulkan/vulkan.h>
#include <unordered_set>
#include <iostream>
#include <vector>
#include "../core/HGpuRsrcManager.h"

struct GLFWwindow;

namespace Hedge
{
    class HRenderer;
    class HBaseGuiManager;
    class HScene;
    class HFrameListener;
    class HEventManager;
    struct SceneRenderInfo;

    /*
    * The renderer creates and accesses the temparory GPU resources through the HGpuRenderRsrcControl instead of the
    * GpuRsrcManager, because the renderer is not aware of the frame or swapchain, but we need these temparory
    * resources to exist until this frame doesn't need the corresponding resources (These rsrc are not needed anymore).
    * 
    * As for long persistent gpu buffers like objects' mesh vert buffers and idx buffers. They are managed by render
    * manager, scene logic and GpuRsrcManager. Vert buffers and idx buffers can be very big, so they cannot be created
    * for every frames. However, it's also possible that the frame 0 needs these buffers but some objects are deleted
    * immediately. So, these GPU memory cannot be released immediately. They can only be released after the frame 0
    * finishes.
    * 
    * Q1: Will frequent create and destroy buffer affect the performance? Not so sure since the memory backend doesn't
    * change. It looks like it is highly possible that the performance should be ok.
    */
    struct HGpuRsrcFrameContext
    {
        std::vector<HGpuBuffer*> m_pTmpGpuBuffers;
        std::vector<HGpuImg*>    m_pTmpGpuImgs;
    };

    class HFrameGpuRenderRsrcControl
    {
    public:
        HFrameGpuRenderRsrcControl();
        ~HFrameGpuRenderRsrcControl() {}

        void Init(uint32_t onFlightRsrcCnt, HGpuRsrcManager* pGpuRsrcManager);

        HGpuBuffer* CreateTmpGpuBuffer(VkBufferUsageFlags usage, VmaAllocationCreateFlags vmaFlags, uint32_t bytesNum);
        HGpuImg*    CreateTmpGpuImage();

        void SwitchToFrame(uint32_t frameIdx);

        void CleanupRsrc();

    private:
        void DestroyCtxBuffersImgs(HGpuRsrcFrameContext& ctx);

        uint32_t                          m_curFrameIdx;
        HGpuRsrcManager*                  m_pGpuRsrcManager;
        std::vector<HGpuRsrcFrameContext> m_gpuRsrcFrameCtxs;
    };

    class HRenderManager
    {
    public:
        HRenderManager(HBaseGuiManager* pGuiManager, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HRenderManager();

        void BeginNewFrame();
        void SendIOEvents(HScene& scene, HEventManager& eventManager);
        void RenderCurrentScene(const SceneRenderInfo& sceneRenderInfo);
        void FinalizeSceneAndSwapBuffers();
        virtual void DrawHud(HFrameListener* pFrameListener) = 0;

        bool WindowShouldClose();

        void SetWindowTitle(const std::string& titleStr);

        VkImageView* GetCurrentRenderImgView();
        VkExtent2D   GetCurrentRenderImgExtent();

    protected:
        // GUI
        uint32_t GetCurSwapchainFrameIdx() { return m_curSwapchainFrameIdx; }

        HBaseGuiManager* m_pGuiManager;

        // Render
        uint32_t m_activeRendererIdx;

    private:
        void CreateSwapchainCmdBuffers();
        void CreateGlfwWindowAndVkSurface();
        void HandleResize();

        static void GlfwFramebufferResizeCallback(GLFWwindow* window, int width, int height) 
            { m_frameBufferResize = true; }

        // Swapchain functions
        void CreateSwapchain();
        void CleanupSwapchain();
        void RecreateSwapchain();
        void CreateSwapchainImageViews();
        void CreateSwapchainSynObjs();
        void CreateRenderpass();
        void CreateSwapchainFramebuffer();

        HGpuImgCreateInfo CreateColorTargetHGpuImgInfo(VkExtent2D extent);
        HGpuImgCreateInfo CreateDepthTargetHGpuImgInfo(VkExtent2D extent);

        // Gpu resource
        HGpuRsrcManager* m_pGpuRsrcManager;

        // GLFW and window context
        GLFWwindow*  m_pGlfwWindow;
        static bool  m_frameBufferResize;
        VkSurfaceKHR m_surface;

        // Swapchain information
        VkSurfaceFormatKHR           m_surfaceFormat;
        VkExtent2D                   m_swapchainImageExtent;
        VkSwapchainKHR               m_swapchain;
        std::vector<VkImageView>     m_swapchainImgViews;
        std::vector<VkFramebuffer>   m_swapchainFramebuffers;
        std::vector<VkSemaphore>     m_swapchainImgAvailableSemaphores;
        std::vector<VkSemaphore>     m_swapchainRenderFinishedSemaphores;
        std::vector<VkFence>         m_inFlightFences;
        std::vector<VkCommandBuffer> m_swapchainRenderCmdBuffers;
        VkRenderPass                 m_renderPass; // The render pass for gui rendering.
        uint32_t                     m_curSwapchainFrameIdx;
        uint32_t                     m_swapchainImgCnt;
        uint32_t                     m_acqSwapchainImgIdx;

        // Renderers -- shared GPU resources for different renderers.
        // May need to be moved to the gui render manager since a game normally doesn't have a second renderer.
        std::vector<HRenderer*>     m_pRenderers;
        HFrameGpuRenderRsrcControl  m_frameGpuRenderRsrcController;
        std::vector<HGpuImg*>       m_frameColorRenderResults;
        std::vector<HGpuImg*>       m_frameDepthRenderResults;
        // std::vector<VkImageView*> m_pRenderImgViews;
        std::vector<VkExtent2D>   m_renderImgsExtents;
        // std::vector<GpuResource>  m_idxRendererGpuRsrcs;
        // std::vector<GpuResource>  m_vertRendererGpuRsrcs;
    };
}
