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
        : // m_curSwapchainFrameIdx(0),
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
        // Create a basic PBR renderer
        VkDevice* pDevice = m_pGpuRsrcManager->GetLogicalDevice();
        HRenderer* pPbrRenderer = new HBasicRenderer(*pDevice);
        m_pRenderers.push_back(pPbrRenderer);
        m_activeRendererIdx = 0;

        m_frameColorRenderResults.resize(m_swapchainImgCnt);
        m_frameDepthRenderResults.resize(m_swapchainImgCnt);
        m_renderImgsExtents.resize(m_swapchainImgCnt);

        m_frameGpuRenderRsrcController.Init(m_swapchainImgCnt,
                                            m_pGpuRsrcManager);
    }

    // ================================================================================================================
    HRenderManager::~HRenderManager()
    {
        CleanupSwapchain();

        VkDevice* pVkDevice = m_pGpuRsrcManager->GetLogicalDevice();

        for (uint32_t i = 0; i < m_swapchainImgCnt; i++)
        {
            m_pGpuRsrcManager->DereferGpuImg(m_frameColorRenderResults[i]);
            m_pGpuRsrcManager->DereferGpuImg(m_frameDepthRenderResults[i]);
        }

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
    // TODO: I am thinking the swapchain doesn't really need a m_curSwapchainFrameIdx.
    //       We only need the m_acqSwapchainImgIdx. If the next image is ready, then all the other resources should
    //       also be ready.
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

        // We have to check the size of 3D render area at the beginning of every frame instead of in the recreate
        // swapchain, because the recreate swapchain can only be called when the overall window area is changed.
        // However, it's possible that people drag the 3D rendering layout window, which changes the size of the 3D
        // window.
        VkExtent2D curRenderTargetExtent = m_renderImgsExtents[m_acqSwapchainImgIdx];
        VkExtent2D desiredRenderTargetExtent = m_pGuiManager->GetRenderExtent();
        if ((desiredRenderTargetExtent.width != curRenderTargetExtent.width) ||
            (desiredRenderTargetExtent.height != curRenderTargetExtent.height))
        {
            HGpuImg* pOldColorRenderResultGpuImg = m_frameColorRenderResults[m_acqSwapchainImgIdx];
            HGpuImg* pOldDepthRenderResultGpuImg = m_frameDepthRenderResults[m_acqSwapchainImgIdx];

            m_pGpuRsrcManager->DereferGpuImg(pOldColorRenderResultGpuImg);
            m_pGpuRsrcManager->DereferGpuImg(pOldDepthRenderResultGpuImg);

            VkExtent3D desiredExtent3D{ desiredRenderTargetExtent.width, desiredRenderTargetExtent.height, 1 };

            HGpuImgCreateInfo colorRenderTarget = CreateColorTargetHGpuImgInfo(desiredRenderTargetExtent);
            HGpuImgCreateInfo depthRenderTarget = CreateDepthTargetHGpuImgInfo(desiredRenderTargetExtent);

            m_frameColorRenderResults[m_acqSwapchainImgIdx] = m_pGpuRsrcManager->CreateGpuImage(colorRenderTarget);
            m_frameDepthRenderResults[m_acqSwapchainImgIdx] = m_pGpuRsrcManager->CreateGpuImage(depthRenderTarget);
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
        const SceneRenderInfo& sceneRenderInfo)
    {
        // Update UBO data and point light storage data.
        m_frameGpuRenderRsrcController.SwitchToFrame(m_acqSwapchainImgIdx);

        // The scene information data.
        uint32_t fragPushConstantDataBytesCnt = sizeof(float) * 4 + sizeof(uint32_t);
        void* pFragPushConstantData = malloc(fragPushConstantDataBytesCnt);
        memset(pFragPushConstantData, 0, fragPushConstantDataBytesCnt);
        memcpy(pFragPushConstantData, sceneRenderInfo.cameraPos, sizeof(float) * 3);

        uint32_t ptLightsCnt = sceneRenderInfo.pointLightsPositions.size();
        memcpy(static_cast<char*>(pFragPushConstantData) + sizeof(float) * 4, &ptLightsCnt, sizeof(ptLightsCnt));

        HGpuBuffer* pSceneInfoUbo = m_frameGpuRenderRsrcController.CreateInitTmpGpuBuffer(
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
                                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                              pFragPushConstantData, fragPushConstantDataBytesCnt);
        free(pFragPushConstantData);

        // The point light data
        uint32_t pointLightPosRadianceBytesCnt = sizeof(HVec3) * sceneRenderInfo.pointLightsPositions.size();
        
        HGpuBuffer* pPtLightsPosStorageBuffer = m_frameGpuRenderRsrcController.CreateInitTmpGpuBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            (void*)sceneRenderInfo.pointLightsPositions.data(), pointLightPosRadianceBytesCnt
        );

        ShaderInputBinding ptLightsPosBinding{ HGPU_BUFFER, pPtLightsPosStorageBuffer };

        HGpuBuffer* pPtLightsRadianceStorageBuffer = m_frameGpuRenderRsrcController.CreateInitTmpGpuBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            (void*)sceneRenderInfo.pointLightsRadiances.data(), pointLightPosRadianceBytesCnt
        );

        ShaderInputBinding ptLightsRadianceBinding{ HGPU_BUFFER, pPtLightsRadianceStorageBuffer };

        // Image based lightning bindings
        // Diffuse cubemap light map
        ShaderInputBinding diffuseLightCubemapBinding{ HGPU_IMG, (void*)&sceneRenderInfo.diffuseCubemapGpuImg };

        // Prefilter environment cubemap
        ShaderInputBinding prefilterEnvCubemapBinding{ HGPU_IMG, (void*)&sceneRenderInfo.prefilterEnvCubemapGpuImg };

        // Environment BRDF
        ShaderInputBinding envBrdfBinding{ HGPU_IMG, (void*)&sceneRenderInfo.envBrdfGpuImg };

        // Fill the command buffer
        VkCommandBuffer curCmdBuffer = m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx];

        VkCommandBufferBeginInfo beginInfo{};
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        }
        VK_CHECK(vkBeginCommandBuffer(curCmdBuffer, &beginInfo));

        uint32_t objsCnt = sceneRenderInfo.modelMats.size();
        if (objsCnt != 0)
        {
            for (uint32_t objIdx = 0; objIdx < objsCnt; objIdx++)
            {
                // The model matrix and view-perspective matrix UBO data.
                uint32_t vertUboDataBytesCnt = 32 * sizeof(float);
                void* pVertUboData = malloc(vertUboDataBytesCnt);

                HMat4x4 modelMat = sceneRenderInfo.modelMats[objIdx];
                HMat4x4 vpMat    = sceneRenderInfo.vpMat;

                memcpy(pVertUboData, modelMat.eles, sizeof(HMat4x4));
                memcpy(static_cast<char*>(pVertUboData) + sizeof(HMat4x4), vpMat.eles, sizeof(HMat4x4));

                HGpuBuffer* pVertUbo = m_frameGpuRenderRsrcController.CreateInitTmpGpuBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                                             VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
                                                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                                                             pVertUboData, vertUboDataBytesCnt);

                free(pVertUboData);
                ShaderInputBinding vertUboBinding{ HGPU_BUFFER, pVertUbo };

                // Base color
                ShaderInputBinding baseColorBinding{ HGPU_IMG, (void*)&sceneRenderInfo.modelBaseColors[objIdx] };

                // Normal map
                ShaderInputBinding normalMapBinding{ HGPU_IMG, (void*)&sceneRenderInfo.modelNormalTexs[objIdx] };

                // Metallic roughness
                ShaderInputBinding metallicRoughnessBinding{ HGPU_IMG, (void*)&sceneRenderInfo.modelMetallicRoughnessTexs[objIdx] };

                // Occlusion
                ShaderInputBinding occlusionBinding{ HGPU_IMG, (void*)&sceneRenderInfo.modelOcclusionTexs[objIdx] };

                HRenderContext renderCtx{};
                {
                    renderCtx.idxBuffer = sceneRenderInfo.objsIdxBuffers[objIdx];
                    renderCtx.idxCnt = sceneRenderInfo.idxCounts[objIdx];
                    renderCtx.vertBuffer = sceneRenderInfo.objsVertBuffers[objIdx];
                    renderCtx.renderArea.offset = { 0, 0 };
                    renderCtx.renderArea.extent = m_pGuiManager->GetRenderExtent();

                    renderCtx.pPushConstantData = pFragPushConstantData;
                    renderCtx.pushConstantDataBytesCnt = fragPushConstantDataBytesCnt;
                    
                    renderCtx.bindings.push_back(vertUboBinding);
                    renderCtx.bindings.push_back(diffuseLightCubemapBinding);
                    renderCtx.bindings.push_back(prefilterEnvCubemapBinding);
                    renderCtx.bindings.push_back(envBrdfBinding);
                    renderCtx.bindings.push_back(baseColorBinding);
                    renderCtx.bindings.push_back(normalMapBinding);
                    renderCtx.bindings.push_back(metallicRoughnessBinding);
                    renderCtx.bindings.push_back(occlusionBinding);

                    renderCtx.colorAttachmentImgView = m_frameColorRenderResults[m_curSwapchainFrameIdx]->gpuImgView;
                    renderCtx.depthAttachmentImgView = m_frameDepthRenderResults[m_curSwapchainFrameIdx]->gpuImgView;
                }

                m_pRenderers[m_activeRendererIdx]->CmdRenderInsts(curCmdBuffer, &renderCtx);
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

        vkGetSwapchainImagesKHR(*m_pGpuRsrcManager->GetLogicalDevice(), m_swapchain, &m_swapchainImgCnt, nullptr);

        // Create the color render target and the depth render target
        VkExtent2D desiredRenderTargetExtent = m_pGuiManager->GetRenderExtent();
        HGpuImgCreateInfo colorRenderTargetInfo = CreateColorTargetHGpuImgInfo(desiredRenderTargetExtent);
        HGpuImgCreateInfo depthRenderTargetInfo = CreateDepthTargetHGpuImgInfo(desiredRenderTargetExtent);

        for (uint32_t i = 0; i < m_swapchainImgCnt; i++)
        {
            m_renderImgsExtents[i] = desiredRenderTargetExtent;
            m_frameColorRenderResults[i] = m_pGpuRsrcManager->CreateGpuImage(colorRenderTargetInfo);
            m_frameDepthRenderResults[i] = m_pGpuRsrcManager->CreateGpuImage(depthRenderTargetInfo);
        }
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainImageViews()
    {
        // Create image views for the swapchain images
        std::vector<VkImage> swapchainImages;
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
    HGpuImgCreateInfo HRenderManager::CreateColorTargetHGpuImgInfo(
        VkExtent2D extent)
    {
        VkExtent3D desiredExtent3D{ extent.width, extent.height, 1 };

        HGpuImgCreateInfo colorRenderTargetInfo{};
        {
            colorRenderTargetInfo.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            colorRenderTargetInfo.hasSampler = false;
            colorRenderTargetInfo.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
            colorRenderTargetInfo.imgFormat = m_surfaceFormat.format;
            colorRenderTargetInfo.imgExtent = desiredExtent3D;
            colorRenderTargetInfo.imgUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            VkImageSubresourceRange imgSubRsrcRange{};
            {
                imgSubRsrcRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imgSubRsrcRange.baseArrayLayer = 0;
                imgSubRsrcRange.layerCount = 1;
                imgSubRsrcRange.baseMipLevel = 0;
                imgSubRsrcRange.levelCount = 1;
            }
            colorRenderTargetInfo.imgSubresRange = imgSubRsrcRange;
        }

        return colorRenderTargetInfo;
    }

    // ================================================================================================================
    HGpuImgCreateInfo HRenderManager::CreateDepthTargetHGpuImgInfo(
        VkExtent2D extent)
    {
        VkExtent3D desiredExtent3D{ extent.width, extent.height, 1 };

        HGpuImgCreateInfo depthRenderTarget{};
        {
            depthRenderTarget.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            depthRenderTarget.hasSampler = false;
            depthRenderTarget.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
            depthRenderTarget.imgFormat = VK_FORMAT_D16_UNORM;
            depthRenderTarget.imgExtent = desiredExtent3D;
            depthRenderTarget.imgUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            VkImageSubresourceRange imgSubRsrcRange{};
            {
                imgSubRsrcRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                imgSubRsrcRange.baseArrayLayer = 0;
                imgSubRsrcRange.layerCount = 1;
                imgSubRsrcRange.baseMipLevel = 0;
                imgSubRsrcRange.levelCount = 1;
            }
            depthRenderTarget.imgSubresRange = imgSubRsrcRange;
        }

        return depthRenderTarget;
    }

    // ================================================================================================================
    HFrameGpuRenderRsrcControl::HFrameGpuRenderRsrcControl()
        : m_pGpuRsrcManager(nullptr),
          m_curFrameIdx(0)
    {
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::Init(
        uint32_t         onFlightRsrcCnt,
        HGpuRsrcManager* pGpuRsrcManager)
    {
        m_gpuRsrcFrameCtxs.resize(onFlightRsrcCnt);
        m_pGpuRsrcManager = pGpuRsrcManager;

        VkDescriptorPool* pDescriptorPool = pGpuRsrcManager->GetDescriptorPool();
        VkDevice*         pDevice         = pGpuRsrcManager->GetLogicalDevice();
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::AddGpuBufferReferControl(
        HGpuBuffer* pHGpuBuffer)
    {
        m_gpuRsrcFrameCtxs[m_curFrameIdx].m_pTmpGpuBuffers.push_back(pHGpuBuffer);
    }

    // ================================================================================================================
    void HFrameGpuRenderRsrcControl::AddGpuImgReferControl(
        HGpuImg* pHGpuImg)
    {
        m_gpuRsrcFrameCtxs[m_curFrameIdx].m_pTmpGpuImgs.push_back(pHGpuImg);
    }

    // ================================================================================================================
    HGpuBuffer* HFrameGpuRenderRsrcControl::CreateInitTmpGpuBuffer(
        VkBufferUsageFlags       usage,
        VmaAllocationCreateFlags vmaFlags,
        void*                    pRamData,
        uint32_t                 bytesNum)
    {
        HGpuBuffer* pBuffer = m_pGpuRsrcManager->CreateGpuBuffer(usage, vmaFlags, bytesNum);
        HGpuRsrcFrameContext& ctx = m_gpuRsrcFrameCtxs[m_curFrameIdx];

        m_pGpuRsrcManager->SendDataToBuffer(pBuffer, pRamData, bytesNum);

        ctx.m_pTmpGpuBuffers.push_back(pBuffer);
        return pBuffer;
    }

    // ================================================================================================================
    HGpuImg* HFrameGpuRenderRsrcControl::CreateInitTmpGpuImage()
    {
        // TODO: Implement as needed.
        // HGpuImg* pImg = m_pGpuRsrcManager->CreateGpuImage();
    }

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