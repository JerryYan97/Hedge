#pragma once

#include "core/HFrameListener.h"

namespace DearImGuiExt
{
    class CustomLayout;
};

namespace Hedge
{
    class HedgeEditor : public HFrameListener
    {
    public:
        HedgeEditor();
        ~HedgeEditor();

        void BuildGame(const char* pPathFileName);

        void CreateGameProject(const char* pPath);

        void Run();

        virtual void FrameStarted();
        virtual void FrameEnded();

    private:
        DearImGuiExt::CustomLayout* myLayout;
    };
}
