#pragma once
#include "HRenderer.h"

namespace Hedge
{
    class HCubemapRendererPipeline : public HPipeline
    {
    public:
        HCubemapRendererPipeline();
        ~HCubemapRendererPipeline();

    protected:
        virtual void CreateSetCustomPipelineInfo() override;

    private:
        void CreateSetDescriptorSetLayouts();
        void CreateSetCubemapPipelineLayout();
        // VkPipelineVertexInputStateCreateInfo CreatePipelineVertexInputInfo();
        // VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo();

        static const VkFormat m_colorAttachmentFormat = VK_FORMAT_R8G8B8A8_SRGB;
    };

    class HCubemapRenderer : public HRenderer
    {
    public:
        explicit HCubemapRenderer(VkDevice device);

        virtual ~HCubemapRenderer();

        virtual void CmdRenderInsts(VkCommandBuffer&            cmdBuf,
                                    const HRenderContext* const pRenderCtx,
                                    const SceneRenderInfo&      sceneRenderInfo,
                                    HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl) override;

    protected:

    private:
        std::vector<ShaderInputBinding> GenPerFrameGpuRsrcBindings(const SceneRenderInfo& sceneRenderInfo,
                                                                   HFrameGpuRenderRsrcControl* pFrameGpuRsrcControl);

    };
}