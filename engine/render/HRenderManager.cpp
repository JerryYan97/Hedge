#include "HRenderManager.h"
#include "HRenderer.h"
#include "../logging/HLogger.h"
#include "Utils.h"
#include "HBaseGuiManager.h"
#include "../scene/HScene.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <set>

static void CheckVkResult(
    VkResult err)
{
    VK_CHECK(err);
}

namespace Hedge
{
    bool HRenderManager::m_frameBufferResize = false;

    // ================================================================================================================
    HRenderManager::HRenderManager(
        HBaseGuiManager* pGuiManager,
        HGpuRsrcManager* pGpuRsrcManager)
        : m_curSwapchainFrameIdx(0),
          m_pGuiManager(pGuiManager),
          m_pGpuRsrcManager(pGpuRsrcManager)
    {
        // Create vulkan instance and possible debug initialization.
        m_pGpuRsrcManager->CreateVulkanAppInstDebugger();

        // Init glfw window and create vk surface from it.
        CreateGlfwWindowAndVkSurface();

        // Create physical device and logical device.
        m_pGpuRsrcManager->CreateVulkanPhyLogicalDevice(&m_surface);

        // Create other basic and shared graphics widgets
        m_pGpuRsrcManager->CreateCommandPool();
        m_pGpuRsrcManager->CreateDescriptorPool();
        m_pGpuRsrcManager->CreateVmaObjects();

        // Create swapchain related objects
        CreateSwapchain();
        CreateSwapchainImageViews();
        CreateSwapchainSynObjs();
        CreateSwapchainCmdBuffers();
        CreateRenderpass();
        CreateSwapchainFramebuffer();

        m_pGuiManager->Init(m_pGlfwWindow,
                            m_pGpuRsrcManager->GetVkInstance(),
                            m_pGpuRsrcManager->GetPhysicalDevice(),
                            m_pGpuRsrcManager->GetLogicalDevice(),
                            m_pGpuRsrcManager->GetGfxQueueFamilyIdx(),
                            m_pGpuRsrcManager->GetGfxQueue(),
                            m_pGpuRsrcManager->GetGfxCmdPool(),
                            m_pGpuRsrcManager->GetDescriptorPool(),
                            m_swapchainImgCnt,
                            &m_renderPass,
                            CheckVkResult);

        // TODO: Let image resource also create and destroy in the gpu rsrc manager instead of passing in the vma allocator.
        m_pRenderers.push_back(new HBasicRenderer(m_swapchainImgCnt, 
                                                  m_pGpuRsrcManager->GetLogicalDevice(), 
                                                  m_surfaceFormat.format, 
                                                  m_pGpuRsrcManager->GetVmaAllocator(),
                                                  m_pGpuRsrcManager));
        m_activeRendererIdx = 0;

        m_frameColorRenderResults.resize(m_swapchainImgCnt);
        // m_renderImgsExtents.resize(m_swapchainImgCnt);
        // m_idxRendererGpuRsrcs.resize(m_swapchainImgCnt);
        // m_vertRendererGpuRsrcs.resize(m_swapchainImgCnt);
    }

    // ================================================================================================================
    HRenderManager::~HRenderManager()
    {
        CleanupSwapchain();

        VkDevice* pVkDevice = m_pGpuRsrcManager->GetLogicalDevice();

        for (auto itr : m_swapchainImgAvailableSemaphores)
        {
            vkDestroySemaphore(*pVkDevice, itr, nullptr);
        }

        for (auto itr : m_swapchainRenderFinishedSemaphores)
        {
            vkDestroySemaphore(*pVkDevice, itr, nullptr);
        }

        for (auto itr : m_inFlightFences)
        {
            vkDestroyFence(*pVkDevice, itr, nullptr);
        }

        for (auto itr : m_pRenderers)
        {
            delete itr;
        }

        for (auto itr : m_frameColorRenderResults)
        {
            m_pGpuRsrcManager->DereferGpuImg(itr);
        }
        
        vkDestroyRenderPass(*pVkDevice, m_renderPass, nullptr);

        vkDestroySurfaceKHR(*m_pGpuRsrcManager->GetVkInstance(), m_surface, nullptr);

        glfwDestroyWindow(m_pGlfwWindow);

        glfwTerminate();
    }

    // ================================================================================================================
    void HRenderManager::BeginNewFrame()
    {
        glfwPollEvents();

        // Wait for the resources from the possible on flight frame
        vkWaitForFences(*m_pGpuRsrcManager->GetLogicalDevice(), 
                        1, 
                        &m_inFlightFences[m_curSwapchainFrameIdx], 
                        VK_TRUE, 
                        UINT64_MAX);

        vkResetFences(*m_pGpuRsrcManager->GetLogicalDevice(), 1, &m_inFlightFences[m_curSwapchainFrameIdx]);
        vkResetCommandBuffer(m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx], 0);

        HandleResize();

        m_pGuiManager->StartNewFrame();
    }

    // ================================================================================================================
    void HRenderManager::SendIOEvents(
        HScene& scene,
        HEventManager& eventManager)
    {
        m_pGuiManager->SendIOEvents(scene, eventManager);
    }

    // ================================================================================================================
    void HRenderManager::HandleResize()
    {
        VkResult result = vkAcquireNextImageKHR(*m_pGpuRsrcManager->GetLogicalDevice(),
                                                m_swapchain, 
                                                UINT64_MAX, 
                                                m_swapchainImgAvailableSemaphores[m_curSwapchainFrameIdx], 
                                                VK_NULL_HANDLE, 
                                                &m_acqSwapchainImgIdx);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // The surface is imcompatiable with the swapchain (resize window).
            RecreateSwapchain();
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            // Not success or usable.
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainCmdBuffers()
    {
        // Create the command buffers for swapchain syn on the graphics pool
        m_swapchainRenderCmdBuffers.resize(m_swapchainImgCnt);
        VkCommandBufferAllocateInfo commandBufferAllocInfo{};
        {
            commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocInfo.commandPool = *m_pGpuRsrcManager->GetGfxCmdPool();
            commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocInfo.commandBufferCount = (uint32_t)m_swapchainRenderCmdBuffers.size();
        }

        VK_CHECK(vkAllocateCommandBuffers(*m_pGpuRsrcManager->GetLogicalDevice(),
                                          &commandBufferAllocInfo,
                                          m_swapchainRenderCmdBuffers.data()));
    }

    // ================================================================================================================
    void HRenderManager::RenderCurrentScene(
        HScene& scene)
    {
        SceneRenderInfo renderInfo = scene.GetSceneRenderInfo();

        // Fill the command buffer
        VkCommandBufferBeginInfo beginInfo{};
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        }
        VK_CHECK(vkBeginCommandBuffer(m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx], &beginInfo));

        if (scene.IsEmpty() == false)
        {
            if (m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pAlloc != nullptr &&
                m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pBuffer != nullptr)
            {
                m_pGpuRsrcManager->DestroyGpuResource(m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx]);
                m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pAlloc = nullptr;
                m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pBuffer = nullptr;
            }

            if (m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pAlloc != nullptr &&
                m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pBuffer != nullptr)
            {
                m_pGpuRsrcManager->DestroyGpuResource(m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx]);
                m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pAlloc = nullptr;
                m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx].m_pBuffer = nullptr;
            }

            if (renderInfo.m_vertBufBytes != 0 && renderInfo.m_idxNum != 0)
            {
                m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx] =
                    m_pGpuRsrcManager->CreateGpuBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, renderInfo.m_vertBufBytes);
                m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx] =
                    m_pGpuRsrcManager->CreateGpuBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(uint32_t) * renderInfo.m_idxNum);


                // Send vertex and index data to gpu buffers
                m_pGpuRsrcManager->SendDataToBuffer(m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx],
                    renderInfo.m_pVert,
                    renderInfo.m_vertBufBytes);

                m_pGpuRsrcManager->SendDataToBuffer(m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx],
                    renderInfo.m_pIdx,
                    renderInfo.m_idxNum * sizeof(uint32_t));

                VkExtent2D renderImgExtent = m_pGuiManager->GetRenderExtent();
                m_pRenderImgViews[m_curSwapchainFrameIdx] = m_pRenderers[m_activeRendererIdx]->Render(
                    m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx],
                    m_idxRendererGpuRsrcs[m_curSwapchainFrameIdx],
                    m_vertRendererGpuRsrcs[m_curSwapchainFrameIdx],
                    renderImgExtent,
                    m_curSwapchainFrameIdx,
                    renderInfo);
                m_renderImgsExtents[m_curSwapchainFrameIdx] = renderImgExtent;
            }
        }
    }

    // ================================================================================================================
    void HRenderManager::FinalizeSceneAndSwapBuffers()
    {
        m_pGuiManager->RecordGuiDraw(m_renderPass,
                                     m_swapchainFramebuffers[m_acqSwapchainImgIdx],
                                     m_swapchainImageExtent,
                                     m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx]);

        VK_CHECK(vkEndCommandBuffer(m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx]));

        // Submit the filled command buffer to the graphics queue to draw the image
        VkSubmitInfo submitInfo{};
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            // This draw would wait at dstStage and wait for the waitSemaphores
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &m_swapchainImgAvailableSemaphores[m_curSwapchainFrameIdx];
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx];
            // This draw would let the signalSemaphore sign when it finishes
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &m_swapchainRenderFinishedSemaphores[m_curSwapchainFrameIdx];
        }
        
        VK_CHECK(vkQueueSubmit(*m_pGpuRsrcManager->GetGfxQueue(), 
                               1, 
                               &submitInfo, 
                               m_inFlightFences[m_curSwapchainFrameIdx]));

        // Put the swapchain into the present info and wait for the graphics queue previously before presenting.
        VkPresentInfoKHR presentInfo{};
        {
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &m_swapchainRenderFinishedSemaphores[m_curSwapchainFrameIdx];
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_swapchain;
            presentInfo.pImageIndices = &m_acqSwapchainImgIdx;
        }
        VkResult result = vkQueuePresentKHR(*m_pGpuRsrcManager->GetPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResize)
        {
            m_frameBufferResize = false;
            RecreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        m_curSwapchainFrameIdx = (m_curSwapchainFrameIdx + 1) % m_swapchainImgCnt;
    }

    // ================================================================================================================
    bool HRenderManager::WindowShouldClose()
    {
        return glfwWindowShouldClose(m_pGlfwWindow);
    }

    // ================================================================================================================
    void HRenderManager::CreateGlfwWindowAndVkSurface()
    {
        // Init glfw window.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pGlfwWindow = glfwCreateWindow(1280, 640, "Untitled -- No root directory", nullptr, nullptr);
        glfwSetFramebufferSizeCallback(m_pGlfwWindow, HRenderManager::GlfwFramebufferResizeCallback);

        // Create vulkan surface from the glfw window.
        VK_CHECK(glfwCreateWindowSurface(*m_pGpuRsrcManager->GetVkInstance(), m_pGlfwWindow, nullptr, &m_surface));
    }

    // ================================================================================================================
    void HRenderManager::SetWindowTitle(
        const std::string& titleStr)
    {
        glfwSetWindowTitle(m_pGlfwWindow, titleStr.c_str());
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchain()
    {
        // Create the swapchain
        // Qurery surface capabilities.
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_pGpuRsrcManager->GetPhysicalDevice(), 
                                                  m_surface, 
                                                  &surfaceCapabilities);

        // Query surface formates
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pGpuRsrcManager->GetPhysicalDevice(), 
                                             m_surface, 
                                             &surfaceFormatCount, 
                                             nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (surfaceFormatCount != 0)
        {
            vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pGpuRsrcManager->GetPhysicalDevice(),
                                                 m_surface,
                                                 &surfaceFormatCount,
                                                 surfaceFormats.data());
        }

        // Query the present mode
        uint32_t surfacePresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pGpuRsrcManager->GetPhysicalDevice(), 
                                                  m_surface, 
                                                  &surfacePresentModeCount, 
                                                  nullptr);
        std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
        if (surfacePresentModeCount != 0)
        {
            vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pGpuRsrcManager->GetPhysicalDevice(),
                                                      m_surface,
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

        // Choose the surface format that supports VK_FORMAT_B8G8R8A8_SRGB and color space 
        // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        bool foundFormat = false;
        for (auto curFormat : surfaceFormats)
        {
            if (curFormat.format     == VK_FORMAT_B8G8R8A8_SRGB && 
                curFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
        glfwGetFramebufferSize(m_pGlfwWindow, &glfwFrameBufferWidth, &glfwFrameBufferHeight);

        m_swapchainImageExtent = {
            std::clamp(static_cast<uint32_t>(glfwFrameBufferWidth),
                       surfaceCapabilities.minImageExtent.width,
                       surfaceCapabilities.maxImageExtent.width),
            std::clamp(static_cast<uint32_t>(glfwFrameBufferHeight),
                       surfaceCapabilities.minImageExtent.height,
                       surfaceCapabilities.maxImageExtent.height)
        };

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        uint32_t queueFamiliesIndices[] = { m_pGpuRsrcManager->GetGfxQueueFamilyIdx(),
                                            m_pGpuRsrcManager->GetPresentFamilyIdx() };
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        {
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.surface = m_surface;
            swapchainCreateInfo.minImageCount = imageCount;
            swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
            swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
            swapchainCreateInfo.imageExtent = m_swapchainImageExtent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (m_pGpuRsrcManager->GetGfxQueueFamilyIdx() != m_pGpuRsrcManager->GetPresentFamilyIdx())
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
        VK_CHECK(vkCreateSwapchainKHR(*m_pGpuRsrcManager->GetLogicalDevice(),
                                      &swapchainCreateInfo, 
                                      nullptr, 
                                      &m_swapchain));
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainImageViews()
    {
        // Create image views for the swapchain images
        std::vector<VkImage> swapchainImages;
        vkGetSwapchainImagesKHR(*m_pGpuRsrcManager->GetLogicalDevice(), m_swapchain, &m_swapchainImgCnt, nullptr);
        swapchainImages.resize(m_swapchainImgCnt);
        vkGetSwapchainImagesKHR(*m_pGpuRsrcManager->GetLogicalDevice(), 
                                m_swapchain, 
                                &m_swapchainImgCnt, 
                                swapchainImages.data());

        m_swapchainImgViews.resize(m_swapchainImgCnt);

        for (size_t i = 0; i < m_swapchainImgCnt; i++)
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
            VK_CHECK(vkCreateImageView(*m_pGpuRsrcManager->GetLogicalDevice(), 
                                       &createInfo, 
                                       nullptr, 
                                       &m_swapchainImgViews[i]));
        }
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainSynObjs()
    {
        m_swapchainImgAvailableSemaphores.resize(m_swapchainImgCnt);
        m_swapchainRenderFinishedSemaphores.resize(m_swapchainImgCnt);
        m_inFlightFences.resize(m_swapchainImgCnt);

        VkSemaphoreCreateInfo semaphoreInfo{};
        {
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        }

        VkFenceCreateInfo fenceInfo{};
        {
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        for (size_t i = 0; i < m_swapchainImgCnt; i++)
        {
            VK_CHECK(vkCreateSemaphore(*m_pGpuRsrcManager->GetLogicalDevice(), 
                                       &semaphoreInfo, 
                                       nullptr, 
                                       &m_swapchainImgAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(*m_pGpuRsrcManager->GetLogicalDevice(), 
                                       &semaphoreInfo, 
                                       nullptr, 
                                       &m_swapchainRenderFinishedSemaphores[i]));
            VK_CHECK(vkCreateFence(*m_pGpuRsrcManager->GetLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]));
        }
    }

    // ================================================================================================================
    void HRenderManager::CreateRenderpass()
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
        VK_CHECK(vkCreateRenderPass(*m_pGpuRsrcManager->GetLogicalDevice(),
                                    &guiRenderPassInfo,
                                    nullptr,
                                    &m_renderPass));
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainFramebuffer()
    {
        // Create Framebuffer
        m_swapchainFramebuffers.resize(m_swapchainImgViews.size());

        for (uint32_t i = 0; i < m_swapchainImgViews.size(); i++)
        {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &m_swapchainImgViews[i];
            framebufferInfo.width = m_swapchainImageExtent.width;
            framebufferInfo.height = m_swapchainImageExtent.height;
            framebufferInfo.layers = 1;
            VK_CHECK(vkCreateFramebuffer(*m_pGpuRsrcManager->GetLogicalDevice(), 
                                         &framebufferInfo, 
                                         nullptr, 
                                         &m_swapchainFramebuffers[i]));
        }
    }

    // ================================================================================================================
    void HRenderManager::CleanupSwapchain()
    {
        // Cleanup swapchain framebuffers, image views and the swapchain itself.
        for (uint32_t i = 0; i < m_swapchainImgViews.size(); i++)
        {
            vkDestroyFramebuffer(*m_pGpuRsrcManager->GetLogicalDevice(), m_swapchainFramebuffers[i], nullptr);
            vkDestroyImageView(*m_pGpuRsrcManager->GetLogicalDevice(), m_swapchainImgViews[i], nullptr);
        }

        // Destroy the swapchain
        vkDestroySwapchainKHR(*m_pGpuRsrcManager->GetLogicalDevice(), m_swapchain, nullptr);
    }

    // ================================================================================================================
    void HRenderManager::RecreateSwapchain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_pGlfwWindow, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_pGlfwWindow, &width, &height);
            glfwWaitEvents();
        }

        m_pGpuRsrcManager->WaitDeviceIdel();
        CleanupSwapchain();
        CreateSwapchain();
        CreateSwapchainImageViews();
        CreateSwapchainFramebuffer();
    }

    // ================================================================================================================
    VkImageView* HRenderManager::GetCurrentRenderImgView() 
    { 
        return &(m_frameColorRenderResults[m_curSwapchainFrameIdx]->gpuImgView); 
    }

    // ================================================================================================================
    VkExtent2D HRenderManager::GetCurrentRenderImgExtent()
    {
        return { m_frameColorRenderResults[m_curSwapchainFrameIdx]->imgInfo.extent.width,
                 m_frameColorRenderResults[m_curSwapchainFrameIdx]->imgInfo.extent.height };
    }

    // ================================================================================================================
    HFrameGpuRenderRsrcControl::HFrameGpuRenderRsrcControl()
        : m_pGpuRsrcManager(nullptr),
          m_curFrameIdx(0)
    {
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::Init(
        uint32_t onFlightRsrcCnt,
        HGpuRsrcManager* pGpuRsrcManager)
    {
        m_gpuRsrcFrameCtxs.resize(onFlightRsrcCnt);
        m_pGpuRsrcManager = pGpuRsrcManager;
    }

    // ================================================================================================================
    HGpuBuffer* HFrameGpuRenderRsrcControl::CreateTmpGpuBuffer(
        VkBufferUsageFlags       usage,
        VmaAllocationCreateFlags vmaFlags,
        uint32_t                 bytesNum)
    {
        HGpuBuffer* pBuffer = m_pGpuRsrcManager->CreateGpuBuffer(usage, vmaFlags, bytesNum);
        HGpuRsrcFrameContext& ctx = m_gpuRsrcFrameCtxs[m_curFrameIdx];
        ctx.m_pTmpGpuBuffers.push_back(pBuffer);
        return pBuffer;
    }

    // ================================================================================================================
    HGpuImg* HFrameGpuRenderRsrcControl::CreateTmpGpuImage()
    {}

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::DestroyCtxBuffersImgs(
        HGpuRsrcFrameContext& ctx)
    {
        for (HGpuBuffer* pTmpGpuBuffer : ctx.m_pTmpGpuBuffers)
        {
            if (pTmpGpuBuffer != nullptr)
            {
                m_pGpuRsrcManager->DereferGpuBuffer(pTmpGpuBuffer);
            }
        }

        for (HGpuImg* pTmpGpuImg : ctx.m_pTmpGpuImgs)
        {
            if (pTmpGpuImg != nullptr)
            {
                m_pGpuRsrcManager->DereferGpuImg(pTmpGpuImg);
            }
        }

        ctx.m_pTmpGpuBuffers.clear();
        ctx.m_pTmpGpuImgs.clear();
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::SwitchToFrame(
        uint32_t frameIdx)
    {
        m_curFrameIdx = frameIdx;

        // Cleanup the frame idx if there are resources.
        HGpuRsrcFrameContext& ctx = m_gpuRsrcFrameCtxs[frameIdx];

        DestroyCtxBuffersImgs(ctx);
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::CleanupRsrc()
    {
        for (auto& ctx : m_gpuRsrcFrameCtxs)
        {
            DestroyCtxBuffersImgs(ctx);
        }
        m_gpuRsrcFrameCtxs.clear();
    }
}