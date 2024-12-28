#include <iostream>
#include "render/HRenderManager.h"
#include "core/HFrameListener.h"
#include "scene/HScene.h"

// Hide console window in release mode
#ifndef _DEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

extern Hedge::HFrameListener* g_pFrameListener;
extern Hedge::HRenderManager* g_pRenderManager;
extern Hedge::HGpuRsrcManager* g_pGpuRsrcManager;

void main(int argc, char** argv)
{
    std::cout << "Hello From Hedge Engine!" << std::endl;
    
    g_pFrameListener->RegisterSerializeClass();
    g_pFrameListener->AppStarts();

    while (g_pRenderManager->WindowShouldClose() == false &&
           g_pFrameListener->GameShouldClose()   == false)
    {
        // Poll events, resize handling
        g_pRenderManager->BeginNewFrame();

        // Frame listener frame start
        g_pFrameListener->FrameStarted();

        // Issue io events
        g_pRenderManager->SendIOEvents(g_pFrameListener->GetActiveScene(),
                                       g_pFrameListener->GetEventManager());

        g_pFrameListener->EntitiesPreRenderTick();

        // Render current scene (Generate scene rendering command buffer)
        g_pRenderManager->RenderCurrentScene(g_pFrameListener->GetActiveSceneRenderInfo());

        g_pFrameListener->EntitiesPostRenderTick();

        // Frame listener frame end
        g_pFrameListener->FrameEnded();

        // Generate HUD info
        g_pRenderManager->DrawHud(g_pFrameListener);

        // Finalize the scene and swap buffers (Generate HUD rendering command buffer and submit to queue)
        g_pRenderManager->FinalizeSceneAndSwapBuffers();
    }

    g_pGpuRsrcManager->WaitDeviceIdle();
}