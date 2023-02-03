#include "HRenderer.h"
#include "HRenderManager.h"
#include "../logging/HLogger.h"
#include "../scene/HScene.h"

#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <vector>
#include <set>

namespace Hedge
{
    // ================================================================================================================
    HRenderer::HRenderer()
    {

    }

    // ================================================================================================================
    HRenderer::~HRenderer()
    {

    }

    // ================================================================================================================
    HBasicRenderer::HBasicRenderer()
    {
        
    }

    // ================================================================================================================
    HBasicRenderer::~HBasicRenderer()
    {}

    // ================================================================================================================
    void HBasicRenderer::Render(
        GpuResource idxResource, 
        GpuResource vertResource)
    {
        
    }
}