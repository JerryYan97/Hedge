#pragma once

#include "../core/HFrameListener.h"
#include "../core/HGpuRsrcManager.h"
#include "../render/HRenderManager.h"
#include "../render/HBaseGuiManager.h"
#include "../scene/HScene.h"

struct ImFont;

namespace Hedge
{
    class HScene;
    class HAssetRsrcManager;

    class HPongGame : public HFrameListener
    {
    public:
        HPongGame();
        virtual ~HPongGame();

        virtual void FrameStarted() override {};
        virtual void FrameEnded() override {};
        virtual void AppStarts() override;

        virtual HScene& GetActiveScene() override { return *m_pScene; }
        virtual HScene* GetActiveScenePtr() { return m_pScene; }
        virtual SceneRenderInfo GetActiveSceneRenderInfo() override { return m_pScene->GetSceneRenderInfo(); }

    protected:
        virtual void RegisterCustomSerializeClass() override;

        std::string m_gameDir;
        std::string m_gameName;
        HScene*     m_pScene;
    };

    class HGameGuiManager : public HBaseGuiManager
    {
    public:
        HGameGuiManager();
        virtual ~HGameGuiManager();

        virtual void GenerateImGuiData() override {};
        void GenerateImGuiData(VkImageView* resultImgView, VkExtent2D resultImgExtent, uint32_t frameIdx);

        virtual VkExtent2D GetRenderExtent() override;
        virtual void SendIOEvents(HScene& scene, HEventManager& eventManager) override;

        void SetPlayerScore(uint32_t score) { m_playerScore = score; }
        void SetOpponentScore(uint32_t score) { m_opponentScore = score; }
        void ShowPauseGameGui(bool isPlayerWin)
        {
            m_showPauseGameGui = true;
            m_isPlayerWin = isPlayerWin;
        }

    protected:
        virtual void CustomFontInit() override;

    private:
        void GenerateHUDEntityImGuiData();

        void SetupMainGameInputHandling();

        uint32_t m_playerScore;
        uint32_t m_opponentScore;

        ImFont* m_pHighResFont = nullptr;
        ImFont* m_pRuleTxtFont = nullptr;

        bool m_showPauseGameGui = false;
        bool m_isPlayerWin = false;
    };

    class HGameRenderManager : public HRenderManager
    {
    public:
        HGameRenderManager(HBaseGuiManager* pGuiManager, HGpuRsrcManager* pGpuRsrcManager);
        virtual ~HGameRenderManager();

        virtual void DrawHud(HFrameListener* pFrameListener) override;
    };

    class GlobalVariablesRAIIManager
    {
    public:
        GlobalVariablesRAIIManager();
        virtual ~GlobalVariablesRAIIManager();

        HPongGame* GetGame() { return m_pGameTemplate; }
        HGameRenderManager* GetGameRenderManager() { return m_pGameRenderManager; }
        HGameGuiManager* GetGameGuiManager() { return m_pGameGuiManager; }
        HGpuRsrcManager* GetGpuRsrcManager() { return m_pGpuRsrcManager; }
        HAssetRsrcManager* GetAssetRsrcManager() { return m_pAssetRsrcManager; }

    protected:
        virtual void CreateCustomGlobalVariables();

    private:
        HPongGame*          m_pGameTemplate;
        HGameRenderManager* m_pGameRenderManager;
        HGameGuiManager*    m_pGameGuiManager;
        HGpuRsrcManager*    m_pGpuRsrcManager;
        HAssetRsrcManager*  m_pAssetRsrcManager;
    };
}
