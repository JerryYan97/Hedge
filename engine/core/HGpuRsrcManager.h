#pragma once
#include <vulkan/vulkan.h>
#include <unordered_set>

#include "vk_mem_alloc.h"

namespace Hedge
{
    struct HGpuBuffer
    {
        VkBuffer      gpuBuffer;
        VmaAllocation gpuBufferAlloc;
        
        // The buffer data in the RAM. Optional.
        union
        {
            uint16_t* pUint16Data;
            uint32_t* pUint32Data;
            float*    pFloatData;
        };

        uint32_t byteCnt;
    };

    struct HGpuImg
    {
        VkImage           gpuImg;
        VmaAllocation     gpuImgAlloc;
        VkImageCreateInfo imgInfo;
        VkImageView       gpuImgView;
        VkSampler         gpuImgSampler;
    };

    // The HGpuRsrcManager holds the vk instance, devices and hides the vma from other parts of the engine.
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
        HGpuBuffer* CreateGpuBuffer(VkBufferUsageFlags usage, VmaAllocationCreateFlags vmaFlags, uint32_t bytesNum);
        void SendDataToBuffer(const HGpuBuffer* const pGpuBuffer, void* pData, uint32_t bytes);
        void DestroyGpuBufferResource(const HGpuBuffer* const pGpuBuffer);

        HGpuImg* CreateGpuImage() {};
        void DestroyGpuImage() {};

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

        std::unordered_set<void*> m_gpuBuffers;
        std::unordered_set<void*> m_gpuImgs;

#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}