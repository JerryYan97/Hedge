#pragma once
#include "HRenderer.h"

namespace Hedge
{
    // The base gui renderer manages dear imgui context and render data.
    class HBaseGuiManager
    {
    public:
        HBaseGuiManager();

        ~HBaseGuiManager();

        void Init(
            GLFWwindow*      pWindow,
            VkInstance       instance,
            VkPhysicalDevice physicalDevice,
            VkDevice         device,
            uint32_t         graphicsQueueFamilyIdx,
            VkQueue          graphicsQueue,
            VkCommandPool    graphicsCmdPool,
            VkDescriptorPool descriptorPool,
            uint32_t         swapchainImgCnt,
            VkRenderPass     guiRenderPass,
            void             (*CheckVkResultFn)(VkResult err));

        void StartNewFrame();

        void RecordGuiDraw(
            VkRenderPass    guiRenderPass, 
            VkFramebuffer   swapchainFramebuffer, 
            VkExtent2D      swapchainImageExtent,
            VkCommandBuffer gfxCmdBuffer);

        void AddTextureToImGUI(VkDescriptorSet* img_ds,
            VkDevice* pVkDevice,
            VkSampler* pVkSampler,
            VkDescriptorPool* pDescriptorPool,
            VkImageView* pTextureImgView,
            uint32_t frameIdx);

        void ImGUIWindowDataArrange();

        virtual void GenerateImGuiData() = 0;

    protected:
        std::vector<VkDescriptorSet> m_guiImgDescriptors;
        uint32_t m_swapchainImgCnt;

    private:
        // Input information.
        // const VkRenderPass* const m_pRenderPass;
    };
}