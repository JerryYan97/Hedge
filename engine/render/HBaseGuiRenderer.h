#pragma once
#include "HRenderer.h"

namespace Hedge
{
    // The base gui renderer manages swap chain and dear imgui rendering.
    class HBaseGuiRenderer : public HRenderer
    {
    public:
        HBaseGuiRenderer(
            const VkPhysicalDevice* const m_pPhysicalDevice,
            const VkDevice* const         m_pDevice,
            const VkSurfaceKHR* const     m_pSurface,
            GLFWwindow*                   m_pWindow,
            const uint32_t                m_graphicsQueueFamilyIdx,
            const uint32_t                m_presentQueueFamilyIdx);

        ~HBaseGuiRenderer();

        virtual void Render();

        virtual void ImGUIWindowDataArrange() = 0;

    private:
        void CreateSwapchain(
            uint32_t graphicsQueueFamilyIdx,
            uint32_t presentQueueFamilyIdx);

        void CleanupSwapchain();
        void RecreateSwapchain();
        void CreateSwapchainImageViews();
        void CreateSwapchainFramebuffer();
        void CreateRenderpass();

        // Information managed by this base gui renderer.
        VkSurfaceFormatKHR m_surfaceFormat;
        VkExtent2D         m_swapchainImageExtent;
        VkSwapchainKHR     m_swapchain;
        VkImageView*       m_pSwapchainImgViews;
        uint32_t           m_swapchainImgViewsCnt;
        VkRenderPass       m_renderpass;
        VkFramebuffer*     m_pSwapchainFramebuffers;
        uint32_t           m_swapchainCnt;

        // Input information.
        const VkPhysicalDevice* const m_pPhysicalDevice;
        const VkDevice* const         m_pDevice;
        const VkSurfaceKHR* const     m_pSurface;
        GLFWwindow*                   m_pWindow;
    };
}