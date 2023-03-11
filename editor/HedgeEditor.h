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

    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        virtual ~HedgeEditor();

        void BuildGame(const char* pPathFileName);

        // Put current scene into the target folder
        void CreateGameProject(const std::string& rootDir, const std::string& projName);

        void OpenGameProject(const std::string& pathName);

        void Run();

        virtual void FrameStarted() override;
        virtual void FrameEnded() override;
        virtual void AppStarts() override;

        virtual HScene& GetActiveScene() override;

    protected:
        virtual void RegisterCustomSerializeClass() override {};

    private:
        std::vector<HScene*> m_pScenes;
        uint32_t             m_activeScene;
        std::string          m_projFilePath;
        std::string          m_rootDir;
        std::string          m_projName;
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

    private:
        HedgeEditor*              m_pHedgeEditor;
        HedgeEditorGuiManager*    m_pHedgeEditorGuiManager;
        HedgeEditorRenderManager* m_pHedgeEditorRenderManager;
        HGpuRsrcManager*          m_pGpuRsrcManager;
    };
}
