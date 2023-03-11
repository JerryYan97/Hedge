#pragma once
#include "../logging/HLogger.h"
#include "HSerializer.h"
#include "HEvent.h"

namespace Hedge
{
    class HScene;

    class HFrameListener
    {
    public:
        HFrameListener();
        virtual ~HFrameListener();

        // All types of classes that can be stored into the scene need to be registered to a jump table.
        void RegisterSerializeClass();
        virtual void FrameStarted() = 0;
        virtual void FrameEnded()   = 0;
        virtual HScene& GetActiveScene() = 0;
        virtual void AppStarts() = 0;

        HEventManager& GetEventManager() { return m_eventManager; }
        HSerializer& GetSerializer() { return m_serializer; }

    protected:
        // The app needs to registers all custom classes to the serializer if it wants to their instances into the
        // scene chunk file.
        virtual void RegisterCustomSerializeClass() = 0;

        Hedge::HLogger m_logger;

    private:
        HEventManager m_eventManager;
        HSerializer   m_serializer;
    };
}
