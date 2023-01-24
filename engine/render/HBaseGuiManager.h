#pragma once
#include "HRenderer.h"

namespace Hedge
{
    // The base gui renderer manages dear imgui context and render data.
    class HBaseGuiManager
    {
    public:
        HBaseGuiManager(const VkRenderPass* const pRenderpass);

        ~HBaseGuiManager();

        void StartNewFrame();

        virtual void Render();

        virtual void ImGUIWindowDataArrange() = 0;

    private:
        // Input information.
        const VkRenderPass* const m_pRenderPass;
    };
}