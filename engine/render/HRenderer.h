#pragma once
/*
* The hedge render manager holds the gpu context, glfw window context and a set of renderer.
* A renderer is an entity to construct a command buffer or a set of command buffers for rendering.
*/

#include <vulkan/vulkan.h>
#include <iostream>

class GLFWwindow;

namespace Hedge
{
    class HRenderer
    {
    public:
        HRenderer();
        ~HRenderer();

        virtual void Render() = 0;
        virtual void Init() = 0;
    private:

    };
}
