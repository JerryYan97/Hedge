#include "HBaseGuiRenderer.h"
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
    HBaseGuiRenderer::HBaseGuiRenderer(
        const VkRenderPass* const pRenderpass)
        : m_pRenderPass(pRenderpass)
    {
    }

    // ================================================================================================================
    HBaseGuiRenderer::~HBaseGuiRenderer()
    {
    }

    // ================================================================================================================
    void HBaseGuiRenderer::Render()
    {
        // Prepare the Dear ImGUI frame data
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 
    }
}