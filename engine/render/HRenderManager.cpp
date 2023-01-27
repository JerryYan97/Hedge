#define VMA_IMPLEMENTATION
#include "HRenderManager.h"
#include "HRenderer.h"
#include "../logging/HLogger.h"
#include "Utils.h"
#include "HBaseGuiManager.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <set>

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cout << "Callback Warning: " << callback_data->messageIdNumber << ":" << callback_data->pMessageIdName
            << ":" << callback_data->pMessage << std::endl;
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cerr << "Callback Error: " << callback_data->messageIdNumber << ":" << callback_data->pMessageIdName
            << ":" << callback_data->pMessage << std::endl;
    }
    return VK_FALSE;
}
#endif // !NDEBUG

static void CheckVkResult(VkResult err)
{
    VK_CHECK(err);
}

namespace Hedge
{
    bool HRenderManager::m_frameBufferResize = false;

    // ================================================================================================================
    HRenderManager::HRenderManager()
        : m_curSwapchainFrameIdx(0)
    {
        // Create vulkan instance and possible debug initialization.
        CreateVulkanAppInstDebugger();

        // Init glfw window and create vk surface from it.
        CreateGlfwWindowAndVkSurface();

        // Create physical device and logical device.
        CreateVulkanPhyLogicalDevice();

        // Create swapchain related objects
        CreateSwapchain();
        CreateSwapchainImageViews();
        CreateSwapchainSynObjs();
        CreateRenderpass();
        CreateSwapchainFramebuffer();

        // Create other basic and shared graphics widgets
        CreateCommandPoolBuffers();
        CreateDescriptorPool();
        CreateVmaObjects();

        m_pGuiManager = new HBaseGuiManager();
        
        m_pGuiManager->Init(m_pGlfwWindow,
                            m_vkInst,
                            m_vkPhyDevice,
                            m_vkDevice,
                            m_gfxQueueFamilyIdx,
                            m_gfxQueue,
                            m_gfxCmdPool,
                            m_descriptorPool,
                            m_swapchainImgCnt,
                            m_renderPass,
                            CheckVkResult);
    }

    // ================================================================================================================
    HRenderManager::~HRenderManager()
    {
        vkDeviceWaitIdle(m_vkDevice);

        delete m_pGuiManager;

        CleanupSwapchain();

        for (auto itr : m_swapchainImgAvailableSemaphores)
        {
            vkDestroySemaphore(m_vkDevice, itr, nullptr);
        }

        for (auto itr : m_swapchainRenderFinishedSemaphores)
        {
            vkDestroySemaphore(m_vkDevice, itr, nullptr);
        }

        for (auto itr : m_inFlightFences)
        {
            vkDestroyFence(m_vkDevice, itr, nullptr);
        }

        vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);

        vmaDestroyAllocator(m_vmaAllocator);

        vkDestroyRenderPass(m_vkDevice, m_renderPass, nullptr);

        vkDestroyCommandPool(m_vkDevice, m_gfxCmdPool, nullptr);

        vkDestroyDevice(m_vkDevice, nullptr);

        vkDestroySurfaceKHR(m_vkInst, m_surface, nullptr);

#ifndef NDEBUG
        // Destroy debug messenger
        auto fpVkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInst, "vkDestroyDebugUtilsMessengerEXT");
        if (fpVkDestroyDebugUtilsMessengerEXT == nullptr)
        {
            exit(1);
        }
        fpVkDestroyDebugUtilsMessengerEXT(m_vkInst, m_dbgMsger, nullptr);
#endif

        vkDestroyInstance(m_vkInst, nullptr);

        glfwDestroyWindow(m_pGlfwWindow);

        glfwTerminate();
    }

    // ================================================================================================================
    void HRenderManager::BeginNewFrame()
    {
        glfwPollEvents();

        // Wait for the resources from the possible on flight frame
        vkWaitForFences(m_vkDevice, 1, &m_inFlightFences[m_curSwapchainFrameIdx], VK_TRUE, UINT64_MAX);

        vkResetFences(m_vkDevice, 1, &m_inFlightFences[m_curSwapchainFrameIdx]);
        vkResetCommandBuffer(m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx], 0);

        HandleResize();
        m_pGuiManager->StartNewFrame();
    }

    // ================================================================================================================
    void HRenderManager::HandleResize()
    {
        VkResult result = vkAcquireNextImageKHR(m_vkDevice, 
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
    void HRenderManager::CreateCommandPoolBuffers()
    {
        // Create the command pool belongs to the graphics queue
        VkCommandPoolCreateInfo commandPoolInfo{};
        {
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolInfo.queueFamilyIndex = m_gfxQueueFamilyIdx;
        }
        VK_CHECK(vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_gfxCmdPool));

        // Create the command buffers for swapchain syn on the graphics pool
        m_swapchainRenderCmdBuffers.resize(m_swapchainImgCnt);
        VkCommandBufferAllocateInfo commandBufferAllocInfo{};
        {
            commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocInfo.commandPool = m_gfxCmdPool;
            commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocInfo.commandBufferCount = (uint32_t)m_swapchainRenderCmdBuffers.size();
        }
        VK_CHECK(vkAllocateCommandBuffers(m_vkDevice, &commandBufferAllocInfo, m_swapchainRenderCmdBuffers.data()));
    }

    // ================================================================================================================
    void HRenderManager::CreateDescriptorPool()
    {
        // Create the descriptor pool
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info{};
        {
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000 * sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
            pool_info.poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            pool_info.pPoolSizes = poolSizes;
        }
        VK_CHECK(vkCreateDescriptorPool(m_vkDevice, &pool_info, nullptr, &m_descriptorPool));
    }

    // ================================================================================================================
    void HRenderManager::CreateVmaObjects()
    {
        // Create the VMA
        VmaVulkanFunctions vkFuncs = {};
        {
            vkFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
            vkFuncs.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;
        }

        VmaAllocatorCreateInfo allocCreateInfo = {};
        {
            allocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
            allocCreateInfo.physicalDevice = m_vkPhyDevice;
            allocCreateInfo.device = m_vkDevice;
            allocCreateInfo.instance = m_vkInst;
            allocCreateInfo.pVulkanFunctions = &vkFuncs;
        }
        vmaCreateAllocator(&allocCreateInfo, &m_vmaAllocator);
    }

    // ================================================================================================================
    void HRenderManager::RenderCurrentScene()
    {}

    // ================================================================================================================
    void HRenderManager::FinalizeSceneAndSwapBuffers()
    {
        // Fill the command buffer
        VkCommandBufferBeginInfo beginInfo{};
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        }
        VK_CHECK(vkBeginCommandBuffer(m_swapchainRenderCmdBuffers[m_curSwapchainFrameIdx], &beginInfo));

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
        
        VK_CHECK(vkQueueSubmit(m_gfxQueue, 1, &submitInfo, m_inFlightFences[m_curSwapchainFrameIdx]));

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
        VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

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
    void HRenderManager::CreateVulkanAppInstDebugger()
    {
        // Initialize instance and application
        VkApplicationInfo appInfo{};
        {
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Hedge";
            appInfo.applicationVersion = 1;
            appInfo.pEngineName = "HedgeEngine";
            appInfo.engineVersion = 1;
            appInfo.apiVersion = VK_API_VERSION_1_3;
        }

        // Init glfw and get the glfw required extension. NOTE: Initialize GLFW before calling any function that requires initialization.
        glfwInit();
        uint32_t glfwExtCnt = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCnt);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtCnt);

#ifndef NDEBUG
        ValidateDebugExtAndValidationLayer();
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

        // Create the debug callback messenger info for instance create and destroy check.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        {
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debug_utils_messenger_callback;
        }
#endif

        VkInstanceCreateInfo instCreateInfo{};
        {
            instCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifndef NDEBUG
            instCreateInfo.pNext = &debugCreateInfo;
#endif
            instCreateInfo.pApplicationInfo = &appInfo;
            instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instCreateInfo.ppEnabledExtensionNames = extensions.data();
            instCreateInfo.enabledLayerCount = 1;
            instCreateInfo.ppEnabledLayerNames = &validationLayerName;
        }
        VK_CHECK(vkCreateInstance(&instCreateInfo, nullptr, &m_vkInst));

#ifndef NDEBUG
        // Create debug messenger
        auto fpVkCreateDebugUtilsMessengerExt = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInst, "vkCreateDebugUtilsMessengerEXT");
        if (fpVkCreateDebugUtilsMessengerExt == nullptr)
        {
            exit(1);
        }
        VK_CHECK(fpVkCreateDebugUtilsMessengerExt(m_vkInst, &debugCreateInfo, nullptr, &m_dbgMsger));
#endif
    }

    // ================================================================================================================
    bool HRenderManager::WindowShouldClose()
    {
        return glfwWindowShouldClose(m_pGlfwWindow);
    }

    // ================================================================================================================
    void HRenderManager::CreateVulkanPhyLogicalDevice()
    {
        // Enumerate the physicalDevices, select the first one and display the name of it.
        uint32_t phyDeviceCount;
        VK_CHECK(vkEnumeratePhysicalDevices(m_vkInst, &phyDeviceCount, nullptr));
        assert(phyDeviceCount >= 1);
        std::vector<VkPhysicalDevice> phyDeviceVec(phyDeviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(m_vkInst, &phyDeviceCount, phyDeviceVec.data()));
        m_vkPhyDevice = phyDeviceVec[0];
        VkPhysicalDeviceProperties physicalDevProperties;
        vkGetPhysicalDeviceProperties(m_vkPhyDevice, &physicalDevProperties);
        HDG_CORE_INFO("Selected device name: {}", physicalDevProperties.deviceName);

        // Initialize the logical device with the queue family that supports both graphics and present on the physical device
        // Find the queue family indices that supports graphics, compute and present.
        uint32_t queueFamilyPropCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhyDevice, &queueFamilyPropCount, nullptr);
        assert(queueFamilyPropCount >= 1);
        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyPropCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhyDevice, &queueFamilyPropCount, queueFamilyProps.data());

        bool foundGraphics = false;
        bool foundPresent  = false;
        bool foundCompute  = false;
        for (uint32_t i = 0; i < queueFamilyPropCount; ++i)
        {
            if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_gfxQueueFamilyIdx = i;
                foundGraphics = true;
            }
            VkBool32 supportPresentSurface;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhyDevice, i, m_surface, &supportPresentSurface);
            if (supportPresentSurface)
            {
                m_presentQueueFamilyIdx = i;
                foundPresent = true;
            }

            if (queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_computeQueueFamilyIdx = i;
                foundCompute = true;
            }

            if (foundGraphics && foundPresent && foundCompute)
            {
                break;
            }
        }
        assert(foundGraphics && foundPresent && foundCompute);

        // Use the queue family index to initialize the queue create info.
        float queue_priorities[1] = { 0.0 };

        // Queue family index should be unique in vk1.2:
        // https://vulkan.lunarg.com/doc/view/1.2.198.0/windows/1.2-extensions/vkspec.html#VUID-VkDeviceCreateInfo-queueFamilyIndex-02802
        std::set<uint32_t> uniqueQueueFamilies = { m_gfxQueueFamilyIdx, m_presentQueueFamilyIdx, m_computeQueueFamilyIdx };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // We need the swap chain device extension
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        // Assembly the info into the device create info
        VkDeviceCreateInfo deviceInfo{};
        {
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceInfo.enabledExtensionCount = 1;
            deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        // Create the logical device
        VK_CHECK(vkCreateDevice(m_vkPhyDevice, &deviceInfo, nullptr, &m_vkDevice));

        // Get present, graphics and compute queue from the logical device
        vkGetDeviceQueue(m_vkDevice, m_gfxQueueFamilyIdx,     0, &m_gfxQueue);
        vkGetDeviceQueue(m_vkDevice, m_presentQueueFamilyIdx, 0, &m_presentQueue);
        vkGetDeviceQueue(m_vkDevice, m_computeQueueFamilyIdx, 0, &m_computeQueue);
    }

    // ================================================================================================================
    void HRenderManager::CreateGlfwWindowAndVkSurface()
    {
        // Init glfw window.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pGlfwWindow = glfwCreateWindow(1280, 640, "Hedge Default Title", nullptr, nullptr);
        glfwSetFramebufferSizeCallback(m_pGlfwWindow, HRenderManager::GlfwFramebufferResizeCallback);

        // Create vulkan surface from the glfw window.
        VK_CHECK(glfwCreateWindowSurface(m_vkInst, m_pGlfwWindow, nullptr, &m_surface));
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchain()
    {
        // Create the swapchain
        // Qurery surface capabilities.
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhyDevice, m_surface, &surfaceCapabilities);

        // Query surface formates
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhyDevice, m_surface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (surfaceFormatCount != 0)
        {
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhyDevice,
                                                 m_surface,
                                                 &surfaceFormatCount,
                                                 surfaceFormats.data());
        }

        // Query the present mode
        uint32_t surfacePresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhyDevice, m_surface, &surfacePresentModeCount, nullptr);
        std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
        if (surfacePresentModeCount != 0)
        {
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhyDevice,
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
        glfwGetFramebufferSize(m_pGlfwWindow, &glfwFrameBufferWidth, &glfwFrameBufferHeight);

        m_swapchainImageExtent = {
            std::clamp(static_cast<uint32_t>(glfwFrameBufferWidth), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp(static_cast<uint32_t>(glfwFrameBufferHeight), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        uint32_t queueFamiliesIndices[] = { m_gfxQueueFamilyIdx, m_presentQueueFamilyIdx };
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
            if (m_gfxQueueFamilyIdx != m_presentQueueFamilyIdx)
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
        VK_CHECK(vkCreateSwapchainKHR(m_vkDevice, &swapchainCreateInfo, nullptr, &m_swapchain));
    }

    // ================================================================================================================
    void HRenderManager::CreateSwapchainImageViews()
    {
        // Create image views for the swapchain images
        std::vector<VkImage> swapchainImages;
        vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &m_swapchainImgCnt, nullptr);
        swapchainImages.resize(m_swapchainImgCnt);
        vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &m_swapchainImgCnt, swapchainImages.data());

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
            VK_CHECK(vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_swapchainImgViews[i]));
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
            VK_CHECK(vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_swapchainImgAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_swapchainRenderFinishedSemaphores[i]));
            VK_CHECK(vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_inFlightFences[i]));
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
        VK_CHECK(vkCreateRenderPass(m_vkDevice, &guiRenderPassInfo, nullptr, &m_renderPass));
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
            VK_CHECK(vkCreateFramebuffer(m_vkDevice, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]));
        }
    }

    // ================================================================================================================
    void HRenderManager::CleanupSwapchain()
    {
        // Cleanup swapchain framebuffers, image views and the swapchain itself.
        for (uint32_t i = 0; i < m_swapchainImgViews.size(); i++)
        {
            vkDestroyFramebuffer(m_vkDevice, m_swapchainFramebuffers[i], nullptr);
            vkDestroyImageView(m_vkDevice, m_swapchainImgViews[i], nullptr);
        }

        // Destroy the swapchain
        vkDestroySwapchainKHR(m_vkDevice, m_swapchain, nullptr);
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

        vkDeviceWaitIdle(m_vkDevice);
        CleanupSwapchain();
        CreateSwapchain();
        CreateSwapchainImageViews();
        CreateSwapchainFramebuffer();
    }

#ifndef NDEBUG

    // ================================================================================================================
    void HRenderManager::ValidateDebugExtAndValidationLayer()
    {
        // Verify that the debug extension for the callback messenger is supported.
        uint32_t propNum;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propNum, nullptr));
        assert(propNum >= 1);
        std::vector<VkExtensionProperties> props(propNum);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propNum, props.data()));
        for (int i = 0; i < props.size(); ++i)
        {
            if (strcmp(props[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                break;
            }
            if (i == propNum - 1)
            {
                std::cout << "Something went very wrong, cannot find " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << " extension"
                    << std::endl;
                exit(1);
            }
        }

        // Verify that the validation layer for Khronos validation is supported
        uint32_t layerNum;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layerNum, nullptr));
        assert(layerNum >= 1);
        std::vector<VkLayerProperties> layers(layerNum);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layerNum, layers.data()));
        for (uint32_t i = 0; i < layerNum; ++i)
        {
            if (strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName) == 0)
            {
                break;
            }
            if (i == layerNum - 1)
            {
                std::cout << "Something went very wrong, cannot find VK_LAYER_KHRONOS_validation extension" << std::endl;
                exit(1);
            }
        }
    }
#endif
}