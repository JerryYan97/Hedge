#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
*/

#include <vulkan/vulkan.h>
#include <iostream>

class GLFWwindow;

namespace Hedge
{
    class HRenderer;

    class HRenderManager
    {
    public:
        HRenderManager();
        ~HRenderManager();

        // Trigger all renderer's Render() managed by this manager.
        void Render();

    private:
        void CreateVulkanAppInstDebugger();
        void CreateGlfwWindowAndVkSurface();
        void CreateVulkanPhyLogicalDevice();

        static void GlfwFramebufferResizeCallback(GLFWwindow* window, int width, int height) { m_frameBufferResize = true; }

        // Vulkan core objects
        VkInstance       m_vkInst;
        VkPhysicalDevice m_vkPhyDevice;
        VkDevice         m_vkDevice;

        // GLFW and window context
        GLFWwindow* m_pGlfwWindow;
        static bool m_frameBufferResize;
        VkSurfaceKHR m_surface;

        // Logical and physical devices context
        uint32_t m_gfxQueueFamilyIdx;
        uint32_t m_computeQueueFamilyIdx;
        uint32_t m_presentQueueFamilyIdx;

        // Renderers: The order matters.
        HRenderer* m_pRenderers;
        uint32_t   m_rendererCnt;


#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}
