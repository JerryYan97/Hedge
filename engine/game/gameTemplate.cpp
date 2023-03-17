#include "GameTemplate.h"

namespace Hedge
{
    // ================================================================================================================
    HGameTemplate::HGameTemplate()
        : HFrameListener()
    {}

    // ================================================================================================================
    HGameTemplate::~HGameTemplate()
    {
        if (m_pScene != nullptr)
        {
            delete m_pScene;
        }
    }

    // ================================================================================================================
    HGameGuiManager::HGameGuiManager()
        : HBaseGuiManager()
    {}

    // ================================================================================================================
    HGameGuiManager::~HGameGuiManager()
    {}

    // ================================================================================================================
    void HGameGuiManager::GenerateImGuiData()
    {}

    // ================================================================================================================
    VkExtent2D HGameGuiManager::GetRenderExtent()
    {}

    // ================================================================================================================
    void HGameGuiManager::SendIOEvents(
        HScene& scene,
        HEventManager& eventManager)
    {}

    // ================================================================================================================
    HGameRenderManager::HGameRenderManager(
        HBaseGuiManager* pGuiManager,
        HGpuRsrcManager* pGpuRsrcManager)
        : HRenderManager(pGuiManager, pGpuRsrcManager)
    {}

    // ================================================================================================================
    HGameRenderManager::~HGameRenderManager()
    {}

    // ================================================================================================================
    void HGameRenderManager::DrawHud(
        HFrameListener* pFrameListener)
    {}

    // ================================================================================================================
    GlobalVariablesRAIIManager::GlobalVariablesRAIIManager()
    {
        CreateCustomGlobalVariables();
    }

    // ================================================================================================================
    GlobalVariablesRAIIManager::~GlobalVariablesRAIIManager()
    {
        delete m_pGameGuiManager;
        delete m_pGameRenderManager;
        delete m_pGpuRsrcManager;
        delete m_pGameTemplate;
    }

    // ================================================================================================================
    void GlobalVariablesRAIIManager::CreateCustomGlobalVariables()
    {
        m_pGameTemplate = new HGameTemplate();
        m_pGameGuiManager = new HGameGuiManager();
        m_pGpuRsrcManager = new HGpuRsrcManager();
        m_pGameRenderManager = new HGameRenderManager(m_pGameGuiManager, m_pGpuRsrcManager);
    }

    // ================================================================================================================
    
}