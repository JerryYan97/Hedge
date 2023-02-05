#include "HBaseGuiManager.h"
#include "Utils.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <vector>
#include <cassert>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Hedge
{
    // ================================================================================================================
    HBaseGuiManager::HBaseGuiManager()
        : m_pVkDevice(nullptr),
          m_pDescriptorPool(nullptr)
    {}

    // ================================================================================================================
    void HBaseGuiManager::Init(
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
        void              (*CheckVkResultFn)(VkResult err))
    {
        m_pVkDevice = pDevice;
        m_pDescriptorPool = pDescriptorPool;
        m_guiImgSamplers.resize(swapchainImgCnt);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(pWindow, true);
        ImGui_ImplVulkan_InitInfo initInfo{};
        {
            initInfo.Instance = *pInstance;
            initInfo.PhysicalDevice = *pPhysicalDevice;
            initInfo.Device = *pDevice;
            initInfo.QueueFamily = graphicsQueueFamilyIdx;
            initInfo.Queue = *pGraphicsQueue;
            initInfo.DescriptorPool = *pDescriptorPool;
            initInfo.Subpass = 0; // GUI render will use the first subpass.
            initInfo.MinImageCount = swapchainImgCnt;
            initInfo.ImageCount = swapchainImgCnt;
            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            initInfo.CheckVkResultFn = CheckVkResultFn;
        }
        ImGui_ImplVulkan_Init(&initInfo, *pGuiRenderPass);

        // Upload Fonts
        {
            // Use any command queue
            VK_CHECK(vkResetCommandPool(*pDevice, *pGraphicsCmdPool, 0));

            VkCommandBufferAllocateInfo fontUploadCmdBufAllocInfo{};
            {
                fontUploadCmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                fontUploadCmdBufAllocInfo.commandPool = *pGraphicsCmdPool;
                fontUploadCmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                fontUploadCmdBufAllocInfo.commandBufferCount = 1;
            }
            VkCommandBuffer fontUploadCmdBuf;
            VK_CHECK(vkAllocateCommandBuffers(*pDevice, &fontUploadCmdBufAllocInfo, &fontUploadCmdBuf));

            VkCommandBufferBeginInfo begin_info{};
            {
                begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            }
            VK_CHECK(vkBeginCommandBuffer(fontUploadCmdBuf, &begin_info));

            ImGui_ImplVulkan_CreateFontsTexture(fontUploadCmdBuf);

            VkSubmitInfo end_info = {};
            {
                end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                end_info.commandBufferCount = 1;
                end_info.pCommandBuffers = &fontUploadCmdBuf;
            }

            VK_CHECK(vkEndCommandBuffer(fontUploadCmdBuf));

            VK_CHECK(vkQueueSubmit(*pGraphicsQueue, 1, &end_info, VK_NULL_HANDLE));

            VK_CHECK(vkDeviceWaitIdle(*pDevice));
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }

    }

    // ================================================================================================================
    HBaseGuiManager::~HBaseGuiManager()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        for (auto itr : m_guiImgSamplers)
        {
            vkDestroySampler(*m_pVkDevice, itr, nullptr);
        }
    }

    // ================================================================================================================
    void HBaseGuiManager::StartNewFrame()
    {
        // Prepare the Dear ImGUI frame data
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // ================================================================================================================
    void HBaseGuiManager::RecordGuiDraw(
        VkRenderPass    guiRenderPass,
        VkFramebuffer   swapchainFramebuffer,
        VkExtent2D      swapchainImageExtent,
        VkCommandBuffer gfxCmdBuffer)
    {
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

        // Begin the render pass and record relevant commands
        // Link framebuffer into the render pass
        VkRenderPassBeginInfo guiRenderPassBeginInfo{};
        {
            guiRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            guiRenderPassBeginInfo.renderPass = guiRenderPass;
            guiRenderPassBeginInfo.framebuffer = swapchainFramebuffer;
            guiRenderPassBeginInfo.renderArea.offset = { 0, 0 };
            guiRenderPassBeginInfo.renderArea.extent = swapchainImageExtent;
            guiRenderPassBeginInfo.clearValueCount = 1;
            guiRenderPassBeginInfo.pClearValues = &clearColor;
        }
        vkCmdBeginRenderPass(gfxCmdBuffer, &guiRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Record the gui rendering commands. We draw GUI after scene because we don't have depth test. So, GUI
        // should be drawn later or the scene would overlay on the GUI.
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        // Record the GUI rendering commands.
        ImGui_ImplVulkan_RenderDrawData(draw_data, gfxCmdBuffer);

        vkCmdEndRenderPass(gfxCmdBuffer);
    }

    // ================================================================================================================
    void HBaseGuiManager::ImGUIWindowDataArrange()
    {
    }

    void HBaseGuiManager::AddTextureToImGUI(
        VkDescriptorSet* img_ds,
        VkImageView*     pTextureImgView,
        uint32_t         frameIdx)
    {
        // Create the Sampler
        vkDestroySampler(*m_pVkDevice, m_guiImgSamplers[frameIdx], nullptr);
        VkSamplerCreateInfo sampler_info{};
        {
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter = VK_FILTER_LINEAR;
            sampler_info.minFilter = VK_FILTER_LINEAR;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.minLod = -1000;
            sampler_info.maxLod = 1000;
            sampler_info.maxAnisotropy = 1.0f;
        }
        VK_CHECK(vkCreateSampler(*m_pVkDevice, &sampler_info, nullptr, &m_guiImgSamplers[frameIdx]));

        if (m_guiImgDescriptors.size() == m_swapchainImgCnt)
        {
            vkFreeDescriptorSets(*m_pVkDevice, *m_pDescriptorPool, 1, &m_guiImgDescriptors[frameIdx]);
        }

        // Create Descriptor Set using ImGUI's implementation
        *img_ds = ImGui_ImplVulkan_AddTexture(m_guiImgSamplers[frameIdx],
                                              *pTextureImgView,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (m_guiImgDescriptors.size() < m_swapchainImgCnt)
        {
            m_guiImgDescriptors.push_back(*img_ds);
        }
        else
        {
            m_guiImgDescriptors[frameIdx] = *img_ds;
        }
    }
}