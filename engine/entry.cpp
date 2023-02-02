#include <iostream>
#include "render/HRenderManager.h"
#include "logging/HLogger.h"
#include "core/HFrameListener.h"

extern Hedge::HFrameListener* g_pFrameListener;

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
        g_pFrameListener->FrameStarted();

        // Render current scene
        renderManager.RenderCurrentScene(g_pFrameListener->GetActiveScene());

        // Frame listener frame end
        g_pFrameListener->FrameEnded();

        // Finalize the scene and swap buffers
        renderManager.FinalizeSceneAndSwapBuffers();
    }

    delete g_pFrameListener;
}