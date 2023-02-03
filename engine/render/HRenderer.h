#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
*/

#include <vulkan/vulkan.h>
#include <iostream>

struct GLFWwindow;

namespace Hedge
{
    struct SceneRenderInfo;
    struct GpuResource;
    class HRenderManager;

    class HRenderer
    {
    public:
        HRenderer();
        ~HRenderer();

        virtual void Render(GpuResource idxResource, GpuResource vertResource) = 0;
    private:

    };

    class HBasicRenderer : public HRenderer
    {
    public:
        HBasicRenderer();
        ~HBasicRenderer();

        virtual void Render(GpuResource idxResource, GpuResource vertResource) override;

    private:
    };
}
