#pragma once
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace Hedge
{
    struct GpuResource
    {
        VkBuffer* m_pBuffer;
        VmaAllocation* m_pAlloc;
    };

    class HGpuRsrcManager
    {
    public:
        HGpuRsrcManager();
        ~HGpuRsrcManager();

        // Init functions
        void CreateVulkanAppInstDebugger();
        void CreateVulkanPhyLogicalDevice(VkSurfaceKHR* pSurface);

        // Create basic and shared graphics widgets
        void CreateCommandPool();
        void CreateDescriptorPool();
        void CreateVmaObjects();


        // Getting interface
        VkInstance* GetVkInstance() { return &m_vkInst; }
        VkPhysicalDevice* GetPhysicalDevice() { return &m_vkPhyDevice; }
        VkDevice* GetLogicalDevice() { return &m_vkDevice; }
        uint32_t GetGfxQueueFamilyIdx() { return m_gfxQueueFamilyIdx; }
        uint32_t GetPresentFamilyIdx() { return m_presentQueueFamilyIdx; }
        VkQueue* GetGfxQueue() { return &m_gfxQueue; }
        VkQueue* GetPresentQueue() { return &m_presentQueue; }
        VkDescriptorPool* GetDescriptorPool() { return &m_descriptorPool; }
        VkCommandPool* GetGfxCmdPool() { return &m_gfxCmdPool; }
        VmaAllocator* GetVmaAllocator() { return &m_vmaAllocator; }

        void WaitDeviceIdel() { vkDeviceWaitIdle(m_vkDevice); };

        // GPU resource manage functions
        GpuResource CreateGpuBuffer(VkBufferUsageFlags usage, uint32_t bytesNum);

        void DestroyGpuResource(GpuResource rsrc);

    private:
        // Vulkan core objects
        VkInstance       m_vkInst;
        VkPhysicalDevice m_vkPhyDevice;
        VkDevice         m_vkDevice;

        // Shared graphics widgets
        VkCommandPool    m_gfxCmdPool;
        VkDescriptorPool m_descriptorPool;
        VmaAllocator     m_vmaAllocator;

        // Logical and physical devices context
        uint32_t m_gfxQueueFamilyIdx;
        uint32_t m_computeQueueFamilyIdx;
        uint32_t m_presentQueueFamilyIdx;
        VkQueue  m_gfxQueue;
        VkQueue  m_computeQueue;
        VkQueue  m_presentQueue;

#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}