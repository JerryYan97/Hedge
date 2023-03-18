#include "GameTemplate.h"
#include "../scene/HScene.h"


Hedge::GlobalVariablesRAIIManager raiiManager;

Hedge::HFrameListener* g_pFrameListener = raiiManager.GetGame();
Hedge::HRenderManager* g_pRenderManager = raiiManager.GetGameRenderManager();
Hedge::HGpuRsrcManager* g_pGpuRsrcManager = raiiManager.GetGpuRsrcManager();

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
    void HGameTemplate::AppStarts()
    {
        // Read in the game settings
        m_pScene = new HScene();
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
    {
        return VkExtent2D{0, 0};
    }

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