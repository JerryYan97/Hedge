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
    {}

    // ================================================================================================================
    void HBaseGuiManager::Init(
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
        void             (*CheckVkResultFn)(VkResult err))
    {
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
            initInfo.Instance = instance;
            initInfo.PhysicalDevice = physicalDevice;
            initInfo.Device = device;
            initInfo.QueueFamily = graphicsQueueFamilyIdx;
            initInfo.Queue = graphicsQueue;
            initInfo.DescriptorPool = descriptorPool;
            initInfo.Subpass = 0; // GUI render will use the first subpass.
            initInfo.MinImageCount = swapchainImgCnt;
            initInfo.ImageCount = swapchainImgCnt;
            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            initInfo.CheckVkResultFn = CheckVkResultFn;
        }
        ImGui_ImplVulkan_Init(&initInfo, guiRenderPass);

        // Upload Fonts
        {
            // Use any command queue
            VK_CHECK(vkResetCommandPool(device, graphicsCmdPool, 0));

            VkCommandBufferAllocateInfo fontUploadCmdBufAllocInfo{};
            {
                fontUploadCmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                fontUploadCmdBufAllocInfo.commandPool = graphicsCmdPool;
                fontUploadCmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                fontUploadCmdBufAllocInfo.commandBufferCount = 1;
            }
            VkCommandBuffer fontUploadCmdBuf;
            VK_CHECK(vkAllocateCommandBuffers(device, &fontUploadCmdBufAllocInfo, &fontUploadCmdBuf));

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

            VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE));

            VK_CHECK(vkDeviceWaitIdle(device));
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }

    // ================================================================================================================
    HBaseGuiManager::~HBaseGuiManager()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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
        VkDevice* pVkDevice,
        VkSampler* pVkSampler,
        VkDescriptorPool* pDescriptorPool,
        VkImageView* pTextureImgView,
        uint32_t frameIdx)
    {
        // Create the Sampler
        vkDestroySampler(*pVkDevice, *pVkSampler, nullptr);
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
        VK_CHECK(vkCreateSampler(*pVkDevice, &sampler_info, nullptr, pVkSampler));

        if (m_guiImgDescriptors.size() == m_swapchainImgCnt)
        {
            vkFreeDescriptorSets(*pVkDevice, *pDescriptorPool, 1, &m_guiImgDescriptors[frameIdx]);
        }

        // Create Descriptor Set using ImGUI's implementation
        *img_ds = ImGui_ImplVulkan_AddTexture(*pVkSampler, *pTextureImgView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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