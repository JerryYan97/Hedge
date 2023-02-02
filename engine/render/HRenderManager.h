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

#include "vk_mem_alloc.h"

struct GLFWwindow;

namespace Hedge
{
    class HRenderer;
    class HBaseGuiManager;
    class HScene;

    class HRenderManager
    {
    public:
        HRenderManager();
        ~HRenderManager();

        void BeginNewFrame();
        void RenderCurrentScene(const HScene& scene);
        void FinalizeSceneAndSwapBuffers();
        void DrawHud();

        bool WindowShouldClose();

    private:
        void CreateVulkanAppInstDebugger();
        void CreateGlfwWindowAndVkSurface();
        void CreateVulkanPhyLogicalDevice();

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

        // Create basic and shared graphics widgets
        void CreateCommandPoolBuffers();
        void CreateDescriptorPool();
        void CreateVmaObjects();

        // Vulkan core objects
        VkInstance       m_vkInst;
        VkPhysicalDevice m_vkPhyDevice;
        VkDevice         m_vkDevice;

        // GLFW and window context
        GLFWwindow*  m_pGlfwWindow;
        static bool  m_frameBufferResize;
        VkSurfaceKHR m_surface;

        // Logical and physical devices context
        uint32_t m_gfxQueueFamilyIdx;
        uint32_t m_computeQueueFamilyIdx;
        uint32_t m_presentQueueFamilyIdx;
        VkQueue  m_gfxQueue;
        VkQueue  m_computeQueue;
        VkQueue  m_presentQueue;

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

        // Shared graphics widgets
        VkCommandPool    m_gfxCmdPool;
        VkDescriptorPool m_descriptorPool;
        VmaAllocator     m_vmaAllocator;

        // Renderers.
        std::vector<HRenderer*> m_pRenderers;
        uint32_t   m_activeRendererIdx;

        // GUI
        HBaseGuiManager* m_pGuiManager;


#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}
