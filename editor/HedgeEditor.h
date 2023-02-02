#pragma once

#include "core/HFrameListener.h"
#include <vector>

namespace DearImGuiExt
{
    class CustomLayout;
};

namespace Hedge
{
    class HScene;

    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        ~HedgeEditor();

        void BuildGame(const char* pPathFileName);

        void CreateGameProject(const char* pPath) {};

        void Run();

        virtual void FrameStarted() override;
        virtual void FrameEnded() override;

        virtual const HScene& GetActiveScene() override;

    private:
        DearImGuiExt::CustomLayout* m_pLayout;
        std::vector<HScene*> m_pScenes;
        uint32_t m_activeScene;
    };
}
