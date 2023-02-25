#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context, swapchain and a set of renderer.
* The manager also manages the sync between renderers for the final rendering result on the screen.
* 
* A renderer is an entity to construct a command buffer or a set of command buffers for a specific rendering result.
* The specific rendering result would be collected by the render manager to build the final rendering result.
*/

#include <vulkan/vulkan.h>
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

    class HRenderManager
    {
    public:
        HRenderManager(HBaseGuiManager* pGuiManager, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HRenderManager();

        void BeginNewFrame();
        void SendIOEvents(HScene& scene, HEventManager& eventManager);
        void RenderCurrentScene(HScene& scene);
        void FinalizeSceneAndSwapBuffers();
        virtual void DrawHud(HFrameListener* pFrameListener) = 0;

        bool WindowShouldClose();

        VkImageView* GetCurrentRenderImgView() { return m_pRenderImgViews[m_curSwapchainFrameIdx]; }
        VkExtent2D   GetCurrentRenderImgExtent() { return m_renderImgsExtents[m_curSwapchainFrameIdx]; }

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
        std::vector<HRenderer*>   m_pRenderers;
        std::vector<VkImageView*> m_pRenderImgViews;
        std::vector<VkExtent2D>   m_renderImgsExtents;
        std::vector<GpuResource>  m_idxRendererGpuRsrcs;
        std::vector<GpuResource>  m_vertRendererGpuRsrcs;
    };
}
