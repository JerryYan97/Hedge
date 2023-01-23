#pragma once
#include "HRenderer.h"

namespace Hedge
{
    // The base gui renderer manages swap chain and dear imgui rendering.
    class HBaseGuiRenderer : public HRenderer
    {
    public:
        HBaseGuiRenderer(const VkRenderPass* const pRenderpass);

        ~HBaseGuiRenderer();

        virtual void Render();

        virtual void ImGUIWindowDataArrange() = 0;

    private:
        // Input information.
        const VkRenderPass* const m_pRenderPass;
    };
}