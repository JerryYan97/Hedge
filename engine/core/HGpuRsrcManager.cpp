#define VMA_IMPLEMENTATION
#include "HGpuRsrcManager.h"
#include <iostream>
#include <vector>
#include <set>
#include <GLFW/glfw3.h>
#include "Utils.h"
#include "../logging/HLogger.h"

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
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

namespace Hedge
{
    // RAII style command buffer and submittion management.
    class HCommandBuffer
    {
    public:
        HCommandBuffer(
            VkDevice      device,
            VkCommandPool cmdPool,
            VkQueue       queue) :
            m_fence(VK_NULL_HANDLE),
            m_queue(queue)
        {
            m_device = device;
            
            VkCommandBufferAllocateInfo commandBufferAllocInfo{};
            {
                commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                commandBufferAllocInfo.commandPool = cmdPool;
                commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                commandBufferAllocInfo.commandBufferCount = 1;
            }
            VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &m_vkCmdBuffer));

            VkFenceCreateInfo fenceInfo{};
            {
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            }
            VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &m_fence));
            VK_CHECK(vkResetFences(device, 1, &m_fence));
        }

        ~HCommandBuffer()
        {
            vkDestroyFence(m_device, m_fence, nullptr);
        }

        VkCommandBuffer GetVkCmdBuffer() { return m_vkCmdBuffer; }

        void SubmitAndWait()
        {
            VkSubmitInfo submitInfo{};
            {
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &m_vkCmdBuffer;
            }
            VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, m_fence));
            VK_CHECK(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX));

            VK_CHECK(vkResetFences(m_device, 1, &m_fence));
            VK_CHECK(vkResetCommandBuffer(m_vkCmdBuffer, 0));
        }

    private:
        VkCommandBuffer m_vkCmdBuffer;
        VkDevice        m_device;
        VkFence         m_fence;
        VkQueue         m_queue;
    };


    // ================================================================================================================
    HGpuRsrcManager::HGpuRsrcManager()
        : m_vkInst(VK_NULL_HANDLE),
          m_vkPhyDevice(VK_NULL_HANDLE),
          m_vkDevice(VK_NULL_HANDLE),
          m_gfxCmdPool(VK_NULL_HANDLE),
          m_descriptorPool(VK_NULL_HANDLE),
          m_vmaAllocator(VK_NULL_HANDLE),
          m_gfxQueueFamilyIdx(0),
          m_computeQueueFamilyIdx(0),
          m_presentQueueFamilyIdx(0),
          m_gfxQueue(VK_NULL_HANDLE),
          m_computeQueue(VK_NULL_HANDLE),
#ifndef NDEBUG
          m_dbgMsger(VK_NULL_HANDLE),
#endif
          m_presentQueue(VK_NULL_HANDLE)

    {}

    // ================================================================================================================
    HGpuRsrcManager::~HGpuRsrcManager()
    {
        // Temp: Clean up gpu rsrc.. Humm... We don't know the type... So we cannot release them...


        vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);

        vmaDestroyAllocator(m_vmaAllocator);

        vkDestroyCommandPool(m_vkDevice, m_gfxCmdPool, nullptr);

        vkDestroyDevice(m_vkDevice, nullptr);

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
    }

    // ================================================================================================================
    void HGpuRsrcManager::CreateCommandPool()
    {
        // Create the command pool belongs to the graphics queue
        VkCommandPoolCreateInfo commandPoolInfo{};
        {
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolInfo.queueFamilyIndex = m_gfxQueueFamilyIdx;
        }
        VK_CHECK(vkCreateCommandPool(m_vkDevice, &commandPoolInfo, nullptr, &m_gfxCmdPool));
    }

    // ================================================================================================================
    void HGpuRsrcManager::ReferGpuBufferImg(
        void* pGpuBufferImg)
    {
        if (m_gpuBuffersImgs.count(pGpuBufferImg) > 0)
        {
            uint32_t dbgRefCnt = std::get<0>(m_gpuBuffersImgs[pGpuBufferImg]) + 1;
            std::get<0>(m_gpuBuffersImgs[pGpuBufferImg]) = dbgRefCnt;
        }
        else
        {
            assert(1, "The refered buffer isn't exist.");
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::DereferGpuBuffer(
        HGpuBuffer* pGpuBuffer)
    {
        if (m_gpuBuffersImgs.count(pGpuBuffer) > 0)
        {
            std::get<0>(m_gpuBuffersImgs[pGpuBuffer]) = std::get<0>(m_gpuBuffersImgs[pGpuBuffer]) - 1;
            if (std::get<0>(m_gpuBuffersImgs[pGpuBuffer]) == 0)
            {
                DestroyGpuBufferResource(pGpuBuffer);
            }
        }
        else
        {
            assert(1, "The derefered buffer isn't exist.");
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::DestroyGpuBufferResource(
        const HGpuBuffer* const pGpuBuffer)
    {
        // Check whether the pointer is valid.
        if (pGpuBuffer != nullptr)
        {
            if (m_gpuBuffersImgs.count((void*)pGpuBuffer) > 0)
            {
                vmaDestroyBuffer(m_vmaAllocator, pGpuBuffer->gpuBuffer, pGpuBuffer->gpuBufferAlloc);
                m_gpuBuffersImgs.erase((void*)pGpuBuffer);
                delete pGpuBuffer;
            }
            else
            {
                assert(1, "The buffer doesn't exist in the gpu buffer set.");
            }
        }
        else
        {
            assert(1, "The buffer cannot be nullptr");
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::CreateDescriptorPool()
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
    void HGpuRsrcManager::CreateVmaObjects()
    {
        // Create the VMA
        VmaVulkanFunctions vkFuncs = {};
        {
            vkFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
            vkFuncs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
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
    void HGpuRsrcManager::CreateVulkanAppInstDebugger()
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

        // Init glfw and get the glfw required extension. NOTE: Initialize GLFW before calling any function that
        // requires initialization.
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
            instCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instCreateInfo.enabledLayerCount = 1;
#endif
            instCreateInfo.pApplicationInfo = &appInfo;
            instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instCreateInfo.ppEnabledExtensionNames = extensions.data();
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
    void HGpuRsrcManager::CreateVulkanPhyLogicalDevice(
        VkSurfaceKHR* pSurface)
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
        bool foundPresent = false;
        bool foundCompute = false;
        for (uint32_t i = 0; i < queueFamilyPropCount; ++i)
        {
            if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_gfxQueueFamilyIdx = i;
                foundGraphics = true;
            }
            VkBool32 supportPresentSurface;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhyDevice, i, *pSurface, &supportPresentSurface);
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
        std::set<uint32_t> uniqueQueueFamilies = { m_gfxQueueFamilyIdx, 
                                                   m_presentQueueFamilyIdx, 
                                                   m_computeQueueFamilyIdx };
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
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
                                                            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME };

        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature{};
        {
            dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
            dynamic_rendering_feature.dynamicRendering = VK_TRUE;
        }

        // Assembly the info into the device create info
        VkDeviceCreateInfo deviceInfo{};
        {
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceInfo.pNext = &dynamic_rendering_feature;
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        // Create the logical device
        VK_CHECK(vkCreateDevice(m_vkPhyDevice, &deviceInfo, nullptr, &m_vkDevice));

        // Get present, graphics and compute queue from the logical device
        vkGetDeviceQueue(m_vkDevice, m_gfxQueueFamilyIdx, 0, &m_gfxQueue);
        vkGetDeviceQueue(m_vkDevice, m_presentQueueFamilyIdx, 0, &m_presentQueue);
        vkGetDeviceQueue(m_vkDevice, m_computeQueueFamilyIdx, 0, &m_computeQueue);
    }

    // ================================================================================================================
    HGpuBuffer* HGpuRsrcManager::CreateGpuBuffer(
        VkBufferUsageFlags       usage,
        VmaAllocationCreateFlags vmaFlags,
        uint32_t                 bytesNum,
        std::string              dbgMsg)
    {
        // Create Buffer and allocate memory for vertex buffer, index buffer and render target.
        VmaAllocationCreateInfo bufAllocInfo = {};
        {
            bufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            bufAllocInfo.flags = vmaFlags;
        }

        // Create Vertex Buffer
        VkBufferCreateInfo bufferInfo = {};
        {
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bytesNum;
            bufferInfo.usage = usage;
        }

        HGpuBuffer* pGpuBuffer = new HGpuBuffer();
        memset(pGpuBuffer, 0, sizeof(HGpuBuffer));

        pGpuBuffer->byteCnt = bytesNum;

        VK_CHECK(vmaCreateBuffer(m_vmaAllocator,
                                 &bufferInfo,
                                 &bufAllocInfo,
                                 &(pGpuBuffer->gpuBuffer),
                                 &(pGpuBuffer->gpuBufferAlloc),
                                 nullptr));

        if (usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            pGpuBuffer->gpuBufferDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        else if (usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            pGpuBuffer->gpuBufferDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }

        pGpuBuffer->gpuBufferDescriptorInfo.buffer = pGpuBuffer->gpuBuffer;
        pGpuBuffer->gpuBufferDescriptorInfo.offset = 0;
        pGpuBuffer->gpuBufferDescriptorInfo.range = bytesNum;

        m_gpuBuffersImgs.insert({ (void*)pGpuBuffer, {1, dbgMsg, HGPU_BUFFER} });

        return pGpuBuffer;
    }

    // ================================================================================================================
    void HGpuRsrcManager::CleanupAllRsrc()
    {
        for (auto& itr : m_gpuBuffersImgs)
        {
            if (std::get<2>(itr.second) == HGPU_BUFFER)
            {
                DestroyGpuBufferResource((HGpuBuffer*)itr.first);
            }
            else
            {
                DestroyGpuImgResource((HGpuImg*)itr.first);
            }
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::SendDataToBuffer(
        const HGpuBuffer* const pGpuBuffer,
        void*                   pData,
        uint32_t                bytes)
    {
        void* mapped = nullptr;
        VK_CHECK(vmaMapMemory(m_vmaAllocator, pGpuBuffer->gpuBufferAlloc, &mapped));
        memcpy(mapped, pData, bytes);
        vmaUnmapMemory(m_vmaAllocator, pGpuBuffer->gpuBufferAlloc);
    }

    // ================================================================================================================
    void HGpuRsrcManager::DereferGpuImg(
        HGpuImg* pGpuImg)
    {
        if (m_gpuBuffersImgs.count(pGpuImg) > 0)
        {
            std::get<0>(m_gpuBuffersImgs[pGpuImg]) = std::get<0>(m_gpuBuffersImgs[pGpuImg]) - 1;
            if (std::get<0>(m_gpuBuffersImgs[pGpuImg]) == 0)
            {
                DestroyGpuImgResource(pGpuImg);
            }
        }
        else
        {
            assert(1, "The derefered image isn't exist.");
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::DestroyGpuImgResource(
        const HGpuImg* const pGpuImg)
    {
        // Check whether the pointer is valid.
        if (pGpuImg != nullptr)
        {
            if (m_gpuBuffersImgs.count((void*)pGpuImg) > 0)
            {
                vmaDestroyImage(m_vmaAllocator, pGpuImg->gpuImg, pGpuImg->gpuImgAlloc);

                vkDestroyImageView(m_vkDevice, pGpuImg->gpuImgView, nullptr);

                vkDestroySampler(m_vkDevice, pGpuImg->gpuImgSampler, nullptr);

                m_gpuBuffersImgs.erase((void*)pGpuImg);
                delete pGpuImg;
            }
            else
            {
                assert(1, "The image doesn't exist in the gpu buffer image set.");
            }
        }
        else
        {
            assert(1, "The image cannot be nullptr");
        }
    }

    // ================================================================================================================
    void HGpuRsrcManager::TransImageLayout(
        HGpuImg*      pTargetImg,
        VkImageLayout targetLayout,
        VkAccessFlags srcAccess,
        VkAccessFlags dstAccess,
        VkPipelineStageFlags srcPipelineStg,
        VkPipelineStageFlags dstPipelineStg)
    {
        if (targetLayout == pTargetImg->curImgLayout)
        {
            return;
        }

        HCommandBuffer hCmdBuffer(m_vkDevice, m_gfxCmdPool, m_gfxQueue);
        VkCommandBuffer cmdBuffer = hCmdBuffer.GetVkCmdBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        }
        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        // Transform the layout of the image to the target layout
        VkImageMemoryBarrier toTargetBarrier{};
        {
            toTargetBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toTargetBarrier.image = pTargetImg->gpuImg;
            toTargetBarrier.subresourceRange = pTargetImg->imgSubresRange;
            toTargetBarrier.srcAccessMask = srcAccess;
            toTargetBarrier.dstAccessMask = dstAccess;
            toTargetBarrier.oldLayout = pTargetImg->curImgLayout;
            toTargetBarrier.newLayout = targetLayout;
        }

        vkCmdPipelineBarrier(
            cmdBuffer,
            srcPipelineStg,
            dstPipelineStg,
            0,
            0, nullptr,
            0, nullptr,
            1, &toTargetBarrier);

        // End the command buffer and submit the packets
        vkEndCommandBuffer(cmdBuffer);

        // Submit the filled command buffer to the graphics queue to draw the image
        hCmdBuffer.SubmitAndWait();
        
        pTargetImg->curImgLayout = targetLayout;
    }

    // ================================================================================================================
    void HGpuRsrcManager::CleanColorGpuImage(
        HGpuImg* pTargetImg,
        VkClearColorValue* pClearColorVal)
    {
        if (m_gpuBuffersImgs.count(pTargetImg) > 0)
        {
            HCommandBuffer hCmdBuffer(m_vkDevice, m_gfxCmdPool, m_gfxQueue);
            VkCommandBuffer cmdBuffer = hCmdBuffer.GetVkCmdBuffer();

            /* Send staging buffer data to the GPU image. */
            VkCommandBufferBeginInfo beginInfo{};
            {
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            }
            VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

            // Transform the layout of the image to copy source
            VkImageMemoryBarrier undefToDstBarrier{};
            {
                undefToDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                undefToDstBarrier.image = pTargetImg->gpuImg;
                undefToDstBarrier.subresourceRange = pTargetImg->imgSubresRange;
                undefToDstBarrier.srcAccessMask = 0;
                undefToDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                undefToDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                undefToDstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &undefToDstBarrier);

            vkCmdClearColorImage(cmdBuffer,
                                 pTargetImg->gpuImg,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 pClearColorVal, 1, &pTargetImg->imgSubresRange);

            // End the command buffer and submit the packets
            vkEndCommandBuffer(cmdBuffer);

            // Submit the filled command buffer to the graphics queue to draw the image
            hCmdBuffer.SubmitAndWait();
        }
        else
        {
            exit(1);
        }
    }

    // ================================================================================================================
    HGpuImg* HGpuRsrcManager::CreateGpuImage(
        HGpuImgCreateInfo createInfo,
        std::string       dbgMsg)
    {
        VmaAllocationCreateInfo imgAllocInfo = {};
        {
            imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            imgAllocInfo.flags = createInfo.allocFlags;
        }

        // Create VkImage
        HGpuImg* pGpuImg = new HGpuImg();
        memset(pGpuImg, 0, sizeof(HGpuImg));

        VkImageCreateInfo imgInfo{};
        {
            imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imgInfo.imageType = VK_IMAGE_TYPE_2D;
            imgInfo.format = createInfo.imgFormat;
            imgInfo.extent = createInfo.imgExtent;
            imgInfo.mipLevels = createInfo.imgSubresRange.levelCount;
            imgInfo.arrayLayers = createInfo.imgSubresRange.layerCount;
            imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imgInfo.usage = createInfo.imgUsageFlags;
            imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imgInfo.flags = createInfo.imgCreateFlags;
        }

        VK_CHECK(vmaCreateImage(m_vmaAllocator,
                                &imgInfo,
                                &imgAllocInfo,
                                &(pGpuImg->gpuImg),
                                &(pGpuImg->gpuImgAlloc),
                                nullptr));

        // Create VkImageView -- Currently, we only assume a 2D image. We may need a cubemap or a 3D image.
        VkImageViewCreateInfo imgViewInfo{};
        {
            imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imgViewInfo.image = pGpuImg->gpuImg;
            imgViewInfo.viewType = createInfo.imgViewType;
            imgViewInfo.format = createInfo.imgFormat;
            imgViewInfo.subresourceRange = createInfo.imgSubresRange;
        }
        
        VK_CHECK(vkCreateImageView(m_vkDevice,
                                   &imgViewInfo,
                                   nullptr,
                                   &(pGpuImg->gpuImgView)));

        if (createInfo.hasSampler)
        {
            VK_CHECK(vkCreateSampler(m_vkDevice, &createInfo.samplerInfo, nullptr, &(pGpuImg->gpuImgSampler)));
        }

        pGpuImg->imgSubresRange = createInfo.imgSubresRange;
        pGpuImg->imgInfo = imgInfo;
        pGpuImg->gpuImgDescriptorInfo.sampler = pGpuImg->gpuImgSampler;
        pGpuImg->gpuImgDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pGpuImg->gpuImgDescriptorInfo.imageView = pGpuImg->gpuImgView;
        pGpuImg->curImgLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // std::cout << "Gpu Img Addr: " << pGpuImg->gpuImg << ". Dbg Msg: " << dbgMsg << std::endl;

        m_gpuBuffersImgs.insert({ (void*)pGpuImg, {1, dbgMsg, HGPU_IMG} });

        return pGpuImg;
    }

    // ================================================================================================================
    VkFence HGpuRsrcManager::CreateFence()
    {
        VkFence fence;

        VkFenceCreateInfo fenceInfo{};
        {
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        VK_CHECK(vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &fence));
        VK_CHECK(vkResetFences(m_vkDevice, 1, &fence));
        
        return fence;
    }

    // ================================================================================================================
    void HGpuRsrcManager::WaitTheFence(
        VkFence fence)
    {
        VK_CHECK(vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, UINT64_MAX));
    }

    // ================================================================================================================
    void HGpuRsrcManager::WaitAndDestroyTheFence(
        VkFence fence)
    {
        VK_CHECK(vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(m_vkDevice, fence, nullptr);
    }

    // ================================================================================================================
    // Let's try whether it's possible to make it from undefined to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
    void HGpuRsrcManager::SendDataToImage(
        HGpuImg*          pGpuImg,
        VkBufferImageCopy bufToImgCopyInfo,
        void*             pData,        
        uint32_t          bytes)
    {
        HCommandBuffer hCmdBuffer(m_vkDevice, m_gfxCmdPool, m_gfxQueue);
        VkCommandBuffer cmdBuffer = hCmdBuffer.GetVkCmdBuffer();

        // Create the staging buffer resources
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufAlloc;
        
        VmaAllocationCreateInfo stagingBufAllocInfo{};
        {
            stagingBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            stagingBufAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        VkBufferCreateInfo stgBufInfo{};
        {
            stgBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stgBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            stgBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stgBufInfo.size = bytes;
        }

        VK_CHECK(vmaCreateBuffer(m_vmaAllocator,
                                 &stgBufInfo,
                                 &stagingBufAllocInfo,
                                 &stagingBuffer,
                                 &stagingBufAlloc, nullptr));

        // Send data to staging Buffer
        void* pStgData;
        vmaMapMemory(m_vmaAllocator, stagingBufAlloc, &pStgData);
        memcpy(pStgData, pData, bytes);
        vmaUnmapMemory(m_vmaAllocator, stagingBufAlloc);

        /* Send staging buffer data to the GPU image. */
        VkCommandBufferBeginInfo beginInfo{};
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        }
        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        // Transform the layout of the image to copy source
        VkImageMemoryBarrier undefToDstBarrier{};
        {
            undefToDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            undefToDstBarrier.image = pGpuImg->gpuImg;
            undefToDstBarrier.subresourceRange = pGpuImg->imgSubresRange;
            undefToDstBarrier.srcAccessMask = 0;
            undefToDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            undefToDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            undefToDstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &undefToDstBarrier);

        // Copy the data from buffer to the image
        vkCmdCopyBufferToImage(
            cmdBuffer,
            stagingBuffer,
            pGpuImg->gpuImg,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &bufToImgCopyInfo);

        // End the command buffer and submit the packets
        vkEndCommandBuffer(cmdBuffer);

        // Submit the filled command buffer to the graphics queue to draw the image
        hCmdBuffer.SubmitAndWait();
        pGpuImg->curImgLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vmaDestroyBuffer(m_vmaAllocator, stagingBuffer, stagingBufAlloc);
    }

#ifndef NDEBUG
    // ================================================================================================================
    void HGpuRsrcManager::ValidateDebugExtAndValidationLayer()
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