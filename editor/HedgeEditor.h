#pragma once

#include "core/HFrameListener.h"
#include "HedgeEditorGuiManager.h"
#include "HedgeEditorRenderManager.h"
#include "core/HGpuRsrcManager.h"
#include <vector>

namespace Hedge
{
    class HScene;
    class HEventManager;
    class HAssetRsrcManager;

    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        virtual ~HedgeEditor();

        // Put current scene into the target folder
        void CreateGameProject(const std::string& rootDir, const std::string& projName);

        void OpenGameProject(const std::string& pathName);

        // Generate cmake file, game config file and solution file
        void BuildDebugGame();

        // Copy and paste the game config file and game exe file to the target directory
        void BuildAndReleaseGame(const std::string& tarDir);

        virtual void FrameStarted() override;
        virtual void FrameEnded() override;
        virtual void AppStarts() override;

        virtual HScene& GetActiveScene() override;

        std::string GetProjectDir() { return m_rootDir; }

    protected:
        virtual void RegisterCustomSerializeClass() override {};

    private:
        std::string GenGameCMakeFileStr(bool isDebug);
        void SaveGameConfig();
        void GenCMakeFile(bool isDebug);

        std::vector<HScene*> m_pScenes;
        uint32_t             m_activeScene;
        std::string          m_projFilePath;
        std::string          m_rootDir;
        std::string          m_projName;
        std::string          m_gameName;
    };

    class GlobalVariablesRAIIManager
    {
    public:
        GlobalVariablesRAIIManager();
        ~GlobalVariablesRAIIManager();

        HedgeEditor* GetHedgeEditor() { return m_pHedgeEditor; }
        HedgeEditorRenderManager* GetHedgeEditorRenderManager() { return m_pHedgeEditorRenderManager; }
        HedgeEditorGuiManager* GetHedgeEditorGuiManager() { return m_pHedgeEditorGuiManager; }
        HGpuRsrcManager* GetGpuRsrcManager() { return m_pGpuRsrcManager; }
        HAssetRsrcManager* GetAssetRsrcManager() { return m_pAssetRsrcManager; }

    private:
        HedgeEditor*              m_pHedgeEditor;
        HedgeEditorGuiManager*    m_pHedgeEditorGuiManager;
        HedgeEditorRenderManager* m_pHedgeEditorRenderManager;
        HGpuRsrcManager*          m_pGpuRsrcManager;
        HAssetRsrcManager*        m_pAssetRsrcManager;
    };
}
