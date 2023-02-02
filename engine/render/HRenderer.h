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

    class HRenderer
    {
    public:
        HRenderer();
        ~HRenderer();

        virtual void Render(const SceneRenderInfo& renderInfo) = 0;
    private:

    };

    class HBasicRenderer : public HRenderer
    {
    public:
        HBasicRenderer();
        ~HBasicRenderer();

        virtual void Render(const SceneRenderInfo& renderInfo) override;
    };
}
