#pragma once
#include "HRenderer.h"
#include "../input/InputHandler.h"
#include <GLFW/glfw3.h>

namespace Hedge
{
    class HScene;
    class HEventManager;

    // The base gui renderer manages dear imgui context and render data.
    class HBaseGuiManager
    {
    public:
        HBaseGuiManager();

        virtual ~HBaseGuiManager();

        void Init(
            GLFWwindow*       pWindow,
            VkInstance*       pInstance,
            VkPhysicalDevice* pPhysicalDevice,
            VkDevice*         pDevice,
            uint32_t          graphicsQueueFamilyIdx,
            VkQueue*          pGraphicsQueue,
            VkCommandPool*    pGraphicsCmdPool,
            VkDescriptorPool* pDescriptorPool,
            uint32_t          swapchainImgCnt,
            VkRenderPass*     pGuiRenderPass,
            void              (*CheckVkResultFn)(VkResult err));

        void StartNewFrame();
        
        void EndFrameWithoutDraw();

        void RecordGuiDraw(
            VkRenderPass    guiRenderPass, 
            VkFramebuffer   swapchainFramebuffer, 
            VkExtent2D      swapchainImageExtent,
            VkCommandBuffer gfxCmdBuffer);

        void AddTextureToImGUI(
            VkDescriptorSet* img_ds,
            VkImageView* pTextureImgView,
            uint32_t frameIdx);

        void AddTextureToImGUI(VkDescriptorSet* img_ds, HGpuImg* pGpuImg);

        void ImGUIWindowDataArrange();

        virtual void GenerateImGuiData() = 0;
        virtual VkExtent2D GetRenderExtent() = 0;

        virtual void SendIOEvents(HScene& scene, HEventManager& eventManager) = 0;

        virtual void AppStart() {}

        void AddOrUpdateCommandGenerator(CommandGenerator* pCommandGenerator) { m_inputHandler.AddOrUpdateCommandGenerator(pCommandGenerator); }
        void RemoveCommandGenerator(CommandGenerator* pCommandGenerator) { m_inputHandler.RemoveCommandGenerator(pCommandGenerator); }

    protected:
        virtual void CustomFontInit() {}

        std::vector<VkDescriptorSet> m_guiImgDescriptors;
        std::vector<VkSampler>       m_guiImgSamplers;
        uint32_t m_swapchainImgCnt;

        ImGuiInputHandler m_inputHandler;

    private:
        // Input information.
        VkDevice* m_pVkDevice;
        VkDescriptorPool* m_pDescriptorPool;
    };
}