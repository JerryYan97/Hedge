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
        
        VkDescriptorBufferInfo gpuBufferDescriptorInfo;
        VkDescriptorType       gpuBufferDescriptorType;
        
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

        VkDescriptorImageInfo gpuImgDescriptorInfo;
        VkImageSubresourceRange  imgSubresRange;
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
        VkImageCreateFlags       imgCreateFlags; // For the cubemap image.

        bool                hasSampler;
        VkSamplerCreateInfo samplerInfo;
    };

    // The HGpuRsrcManager holds the vk instance, devices and hides the vma from other parts of the engine.
    // Vertex and idx buffer: refered and released by static mesh components and the render manager.
    // Textures: refered and released by texture components, material components and the render manager.
    // NOTE: We use HGpuBuffer or HGpuImg pointer address to label a gpu resource, so we have to pass their pointers
    // around instead of their structs.
    // TODO: We maybe able to say GPU resources include both GRAM resources and GPU compute resources.
    // TODO: We need the GUID + String based GPU rsrc management now, because multiple cubes materials need one same
    //       roughness, metallic, occlusion texture (Pure black). -- No. We can just use the asset manager and the
    //       management of the texture assets to workaround it.
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
        void SendDataToImage(const HGpuImg* pGpuImg, VkBufferImageCopy bufToImgCopyInfo, void* pData, uint32_t bytes);

        void CleanColorGpuImage(HGpuImg* pTargetImg, VkClearColorValue* pClearColorVal);
        
        void TransImageLayout(HGpuImg* pTargetImg, VkImageLayout targetLayout);

        VkFence CreateFence();
        void WaitAndDestroyTheFence(VkFence fence);

    private:
        void HGpuRsrcManager::DestroyGpuBufferResource(const HGpuBuffer* const pGpuBuffer);
        void HGpuRsrcManager::DestroyGpuImgResource(const HGpuImg* const pGpuImg);

        // Vulkan core objects
        VkInstance       m_vkInst;
        VkPhysicalDevice m_vkPhyDevice;
        VkDevice         m_vkDevice;

        // Shared graphics widgets
        VkCommandPool    m_gfxCmdPool;
        VkDescriptorPool m_descriptorPool; // The descriptor pool is still needed for the imgui.
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