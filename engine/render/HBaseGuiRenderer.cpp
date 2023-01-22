#include "HBaseGuiRenderer.h"
#include "Utils.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <vector>
#include <cassert>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Hedge
{
    // ================================================================================================================
    HBaseGuiRenderer::HBaseGuiRenderer(
        const VkPhysicalDevice* const pPhysicalDevice,
        const VkDevice* const         pDevice,
        const VkSurfaceKHR* const     pSurface,
        GLFWwindow*                   pWindow,
        const uint32_t                graphicsQueueFamilyIdx,
        const uint32_t                presentQueueFamilyIdx)
        : m_pPhysicalDevice(pPhysicalDevice),
          m_pDevice(pDevice),
          m_pSurface(pSurface),
          m_pWindow(pWindow)
    {
        CreateSwapchain(graphicsQueueFamilyIdx, presentQueueFamilyIdx);
        CreateSwapchainImageViews();
        CreateRenderpass();
    }

    // ================================================================================================================
    HBaseGuiRenderer::~HBaseGuiRenderer()
    {
        CleanupSwapchain();
        vkDestroyRenderPass(*m_pDevice, m_renderpass, nullptr);
    }

    // ================================================================================================================
    void HBaseGuiRenderer::CreateSwapchain(
        uint32_t graphicsQueueFamilyIdx,
        uint32_t presentQueueFamilyIdx)
    {
        // Create the swapchain
        // Qurery surface capabilities.
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_pPhysicalDevice, *m_pSurface, &surfaceCapabilities);

        // Query surface formates
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pPhysicalDevice, *m_pSurface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (surfaceFormatCount != 0)
        {
            vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pPhysicalDevice, 
                                                 *m_pSurface, 
                                                 &surfaceFormatCount, 
                                                 surfaceFormats.data());
        }

        // Query the present mode
        uint32_t surfacePresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pPhysicalDevice, *m_pSurface, &surfacePresentModeCount, nullptr);
        std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
        if (surfacePresentModeCount != 0)
        {
            vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pPhysicalDevice,
                                                      *m_pSurface,
                                                      &surfacePresentModeCount,
                                                      surfacePresentModes.data());
        }

        // Choose the VK_PRESENT_MODE_FIFO_KHR.
        VkPresentModeKHR choisenPresentMode{};
        bool foundMailBoxPresentMode = false;
        for (const auto& avaPresentMode : surfacePresentModes)
        {
            if (avaPresentMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                choisenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
                foundMailBoxPresentMode = true;
                break;
            }
        }
        assert(choisenPresentMode == VK_PRESENT_MODE_FIFO_KHR);

        // Choose the surface format that supports VK_FORMAT_B8G8R8A8_SRGB and color space VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        bool foundFormat = false;
        for (auto curFormat : surfaceFormats)
        {
            if (curFormat.format == VK_FORMAT_B8G8R8A8_SRGB && curFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                foundFormat = true;
                m_surfaceFormat = curFormat;
                break;
            }
        }
        assert(foundFormat);

        // Init swapchain's image extent
        int glfwFrameBufferWidth;
        int glfwFrameBufferHeight;
        glfwGetFramebufferSize(m_pWindow, &glfwFrameBufferWidth, &glfwFrameBufferHeight);

        m_swapchainImageExtent = {
            std::clamp(static_cast<uint32_t>(glfwFrameBufferWidth), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp(static_cast<uint32_t>(glfwFrameBufferHeight), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        uint32_t queueFamiliesIndices[] = { graphicsQueueFamilyIdx, presentQueueFamilyIdx };
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        {
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.surface = *m_pSurface;
            swapchainCreateInfo.minImageCount = imageCount;
            swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
            swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
            swapchainCreateInfo.imageExtent = m_swapchainImageExtent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (graphicsQueueFamilyIdx != presentQueueFamilyIdx)
            {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainCreateInfo.queueFamilyIndexCount = 2;
                swapchainCreateInfo.pQueueFamilyIndices = queueFamiliesIndices;
            }
            else
            {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = choisenPresentMode;
            swapchainCreateInfo.clipped = VK_TRUE;
        }
        VK_CHECK(vkCreateSwapchainKHR(*m_pDevice, &swapchainCreateInfo, nullptr, &m_swapchain));
    }

    // ================================================================================================================
    void HBaseGuiRenderer::CreateSwapchainImageViews()
    {
        // Create image views for the swapchain images
        std::vector<VkImage> swapchainImages;
        uint32_t swapchainImageCount;
        vkGetSwapchainImagesKHR(*m_pDevice, m_swapchain, &swapchainImageCount, nullptr);
        swapchainImages.resize(swapchainImageCount);
        vkGetSwapchainImagesKHR(*m_pDevice, m_swapchain, &swapchainImageCount, swapchainImages.data());

        m_pSwapchainImgViews = new VkImageView[swapchainImageCount];
        m_swapchainImgViewsCnt = swapchainImageCount;

        for (size_t i = 0; i < swapchainImageCount; i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_surfaceFormat.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            VK_CHECK(vkCreateImageView(*m_pDevice, &createInfo, nullptr, &m_pSwapchainImgViews[i]));
        }
    }

    // ================================================================================================================
    void HBaseGuiRenderer::CreateRenderpass()
    {
        // Create the render pass -- We will use dynamic rendering for the scene rendering
        // Specify the GUI attachment: We will need to present everything in GUI. So, the finalLayout would be presentable.
        VkAttachmentDescription guiRenderTargetAttachment{};
        {
            guiRenderTargetAttachment.format = m_surfaceFormat.format;
            guiRenderTargetAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            guiRenderTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            guiRenderTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            guiRenderTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            guiRenderTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            guiRenderTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            guiRenderTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        // Specify the color reference, which specifies the attachment layout during the subpass
        VkAttachmentReference guiAttachmentRef{};
        {
            guiAttachmentRef.attachment = 0;
            guiAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        // Specity the subpass executed for the GUI
        VkSubpassDescription guiSubpass{};
        {
            guiSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            guiSubpass.colorAttachmentCount = 1;
            guiSubpass.pColorAttachments = &guiAttachmentRef;
        }

        // Specify the dependency between the scene subpass (0) and the gui subpass (1).
        // The gui subpass' rendering output should wait for the scene subpass' rendering output.
        VkSubpassDependency guiSubpassesDependency{};
        {
            guiSubpassesDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            guiSubpassesDependency.dstSubpass = 0;
            guiSubpassesDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            guiSubpassesDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            guiSubpassesDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            guiSubpassesDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        // Create the render pass
        VkRenderPassCreateInfo guiRenderPassInfo{};
        {
            guiRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            guiRenderPassInfo.attachmentCount = 1;
            guiRenderPassInfo.pAttachments = &guiRenderTargetAttachment;
            guiRenderPassInfo.subpassCount = 1;
            guiRenderPassInfo.pSubpasses = &guiSubpass;
            guiRenderPassInfo.dependencyCount = 1;
            guiRenderPassInfo.pDependencies = &guiSubpassesDependency;
        }
        VK_CHECK(vkCreateRenderPass(*m_pDevice, &guiRenderPassInfo, nullptr, &m_renderpass));
    }

    // ================================================================================================================
    void HBaseGuiRenderer::CreateSwapchainFramebuffer()
    {
        // Create Framebuffer
        m_pSwapchainFramebuffers = new VkFramebuffer[m_swapchainImgViewsCnt];

        for (uint32_t i = 0; i < m_swapchainImgViewsCnt; i++)
        {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderpass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &m_pSwapchainImgViews[i];
            framebufferInfo.width = m_swapchainImageExtent.width;
            framebufferInfo.height = m_swapchainImageExtent.height;
            framebufferInfo.layers = 1;
            VK_CHECK(vkCreateFramebuffer(*m_pDevice, &framebufferInfo, nullptr, &m_pSwapchainFramebuffers[i]));
        }
    }

    // ================================================================================================================
    void HBaseGuiRenderer::CleanupSwapchain()
    {
        // Cleanup swapchain framebuffers, image views and the swapchain itself.
        for (uint32_t i = 0; i < m_swapchainImgViewsCnt; i++)
        {
            vkDestroyFramebuffer(*m_pDevice, m_pSwapchainFramebuffers[i], nullptr);
            vkDestroyImageView(*m_pDevice, m_pSwapchainImgViews[i], nullptr);
        }

        // Destroy the swapchain
        vkDestroySwapchainKHR(*m_pDevice, m_swapchain, nullptr);
    }

    // ================================================================================================================
    void HBaseGuiRenderer::RecreateSwapchain()
    {}

    // ================================================================================================================
    void HBaseGuiRenderer::Render()
    {
        // Prepare the Dear ImGUI frame data
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


    }
}