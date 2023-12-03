#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>

#include "vk_mem_alloc.h"

namespace Hedge
{
    enum HGpuRsrcType
    {
        HGPU_BUFFER,
        HGPU_IMG
    };

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

    struct HGpuImgCreateInfo
    {
        // VkImageCreateInfo        imgInfo;
        VmaAllocationCreateFlags allocFlags;
        VkImageSubresourceRange  imgSubresRange;
        VkImageViewType          imgViewType;
        VkFormat                 imgFormat;
        VkExtent3D               imgExtent;
        VkImageUsageFlags        imgUsageFlags;

        bool                hasSampler;
        VkSamplerCreateInfo samplerInfo;
    };

    // The HGpuRsrcManager holds the vk instance, devices and hides the vma from other parts of the engine.
    // Vertex and idx buffer: refered and released by static mesh components and the render manager.
    // Textures: refered and released by texture components, material components and the render manager.
    // TODO: We maybe able to say GPU resources include both GRAM resources and GPU compute resources.
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

        // GPU resource manage functions. The users should derefer the buffer or image when it is not needed.
        // Add one more refer counter of this buffer or image
        void ReferGpuBufferImg(void* pGpuBufferImg);

        // Decrease one refer counter of this buffer or image. Release the rsrc if nobody refers it.
        void DereferGpuBuffer(HGpuBuffer* pGpuBuffer);
        void DereferGpuImg(HGpuImg* pGpuImg);
        
        // Create a gpu buffer and add a refer counter of this buffer.
        HGpuBuffer* CreateGpuBuffer(VkBufferUsageFlags usage, VmaAllocationCreateFlags vmaFlags, uint32_t bytesNum);
        void SendDataToBuffer(const HGpuBuffer* const pGpuBuffer, void* pData, uint32_t bytes);

        HGpuImg* CreateGpuImage(HGpuImgCreateInfo createInfo);
        

    private:
        void HGpuRsrcManager::DestroyGpuBufferResource(const HGpuBuffer* const pGpuBuffer);
        void HGpuRsrcManager::DestroyGpuImgResource(const HGpuImg* const pGpuImg);

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

        std::unordered_map<void*, uint32_t> m_gpuBuffersImgs;

#ifndef NDEBUG
        // Debug mode
        void ValidateDebugExtAndValidationLayer();

        VkDebugUtilsMessengerEXT m_dbgMsger;
#endif

    };
}