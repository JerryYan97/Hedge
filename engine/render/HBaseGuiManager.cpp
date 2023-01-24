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
    HBaseGuiManager::HBaseGuiManager(
        const VkRenderPass* const pRenderpass)
        : m_pRenderPass(pRenderpass)
    {
    }

    // ================================================================================================================
    HBaseGuiManager::~HBaseGuiManager()
    {
    }

    // ================================================================================================================
    void HBaseGuiManager::Render()
    {
        // Prepare the Dear ImGUI frame data
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 
    }
}