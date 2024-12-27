#pragma once
#include <chrono>
#include "../logging/HLogger.h"
#include "HSerializer.h"
#include "HEvent.h"

#define HEDGE_ENGINE_MAJOR_VERSION 0
#define HEDGE_ENGINE_MINOR_VERSION 1

namespace Hedge
{
    class HScene;
    struct SceneRenderInfo;

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
        virtual SceneRenderInfo GetActiveSceneRenderInfo() = 0;
        virtual void AppStarts() = 0;
        virtual bool GameShouldClose() { return m_gameShouldClose; }

        void EntitiesPreRenderTick();
        void EntitiesPostRenderTick();

        void SetCloseGame() { m_gameShouldClose = true; }

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

        bool m_gameShouldClose = false;

        std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimeStamp;
        double m_elapsedSec;
    };
}
