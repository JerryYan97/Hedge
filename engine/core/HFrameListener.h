#pragma once
#include "../logging/HLogger.h"
#include "HEvent.h"

namespace Hedge
{
    class HScene;

    class HFrameListener
    {
    public:
        HFrameListener();
        virtual ~HFrameListener();

        virtual void FrameStarted() = 0;
        virtual void FrameEnded()   = 0;
        virtual HScene& GetActiveScene() = 0;

        HEventManager& GetEventManager() { return m_eventManager; }

    protected:
        Hedge::HLogger m_logger;

    private:
        HEventManager m_eventManager;
    };
}
