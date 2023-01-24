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

struct GLFWwindow;

namespace Hedge
{
    class HRenderer;

    class HRenderManager
    {
    public:
        HRenderManager();
        ~HRenderManager();

        void BeginNewFrame();
        void RenderCurrentScene();
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

        void CreateSwapchain();
        void CleanupSwapchain();
        void RecreateSwapchain();
        void CreateSwapchainImageViews();
        void CreateRenderpass();
        void CreateSwapchainFramebuffer();

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

        // Swapchain information
        VkSurfaceFormatKHR         m_surfaceFormat;
        VkExtent2D                 m_swapchainImageExtent;
        VkSwapchainKHR             m_swapchain;
        std::vector<VkImageView>   m_swapchainImgViews;
        std::vector<VkFramebuffer> m_swapchainFramebuffers;
        VkRenderPass               m_renderpass;

        // Renderers.
        HRenderer* m_pRenderers;
        uint32_t   m_activeRendererIdx;


#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}
