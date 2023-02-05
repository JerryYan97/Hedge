#include <iostream>
#include "render/HRenderManager.h"
#include "core/HFrameListener.h"

extern Hedge::HFrameListener* g_pFrameListener;
extern Hedge::HRenderManager* g_pRenderManager;

void main(int argc, char** argv)
{
    std::cout << "Hello From Hedge Engine!" << std::endl;

    while (g_pRenderManager->WindowShouldClose() == false)
    {
        // Poll events, resize handling
        g_pRenderManager->BeginNewFrame();

        // Frame listener frame start
        g_pFrameListener->FrameStarted();

        // Render current scene (Generate scene rendering command buffer)
        g_pRenderManager->RenderCurrentScene(g_pFrameListener->GetActiveScene());

        // Frame listener frame end
        g_pFrameListener->FrameEnded();

        // Generate HUD info
        g_pRenderManager->DrawHud(g_pFrameListener);

        // Finalize the scene and swap buffers (Generate HUD rendering command buffer and submit to queue)
        g_pRenderManager->FinalizeSceneAndSwapBuffers();
    }

    g_pRenderManager->WaitDeviceIdel();
}