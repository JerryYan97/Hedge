#include <iostream>
#include "render/HRenderManager.h"
#include "logging/HLogger.h"

void main(int argc, char** argv)
{
    std::cout << "Hello From Hedge Engine!" << std::endl;

    Hedge::HLogger logger;
    Hedge::HRenderManager renderManager;

    while (renderManager.WindowShouldClose() == false)
    {
        // Poll events, resize handling
        renderManager.BeginNewFrame();

        // Frame listener frame start

        // Render current scene

        // Frame listener frame end

        // Finalize the scene and swap buffers
    }
}