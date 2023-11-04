#pragma once

#include "../core/HFrameListener.h"
#include "../core/HGpuRsrcManager.h"
#include "../render/HRenderManager.h"
#include "../render/HBaseGuiManager.h"

namespace Hedge
{
    class HScene;
    class HAssetRsrcManager;

    class HGameTemplate : public HFrameListener
    {
    public:
        HGameTemplate();
        virtual ~HGameTemplate();

        virtual void FrameStarted() override {};
        virtual void FrameEnded() override {};
        virtual void AppStarts() override;

        virtual HScene& GetActiveScene() override { return *m_pScene; };

    protected:
        virtual void RegisterCustomSerializeClass() override {};

        std::string m_gameName;
        HScene* m_pScene;
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

        HGameTemplate* GetGame() { return m_pGameTemplate; }
        HGameRenderManager* GetGameRenderManager() { return m_pGameRenderManager; }
        HGameGuiManager* GetGameGuiManager() { return m_pGameGuiManager; }
        HGpuRsrcManager* GetGpuRsrcManager() { return m_pGpuRsrcManager; }
        HAssetRsrcManager* GetAssetRsrcManager() { return m_pAssetRsrcManager; }

    protected:
        virtual void CreateCustomGlobalVariables();

    private:
        HGameTemplate* m_pGameTemplate;
        HGameRenderManager* m_pGameRenderManager;
        HGameGuiManager* m_pGameGuiManager;
        HGpuRsrcManager* m_pGpuRsrcManager;
        HAssetRsrcManager* m_pAssetRsrcManager;
    };
}
