#include "HFrameListener.h"
#include "HEntity.h"
#include "Utils.h"
#include "../scene/HScene.h"

namespace Hedge
{
    // ================================================================================================================
    HFrameListener::HFrameListener()
        :m_eventManager(),
         m_elapsedSec(-1.0)
    {}

    // ================================================================================================================
    HFrameListener::~HFrameListener()
    {}

    // ================================================================================================================
    void HFrameListener::RegisterSerializeClass()
    {
        // Register all engine entity classes
        m_serializer.RegisterAClass(crc32("HCubeEntity"), { HCubeEntity::Seralize,
                                                            HCubeEntity::Deseralize,
                                                            HCubeEntity::CreateEntity });
        m_serializer.RegisterAClass(crc32("HCameraEntity"), { HCameraEntity::Seralize,
                                                              HCameraEntity::Deseralize,
                                                              HCameraEntity::CreateEntity });
        m_serializer.RegisterAClass(crc32("HPointLightEntity"), { HPointLightEntity::Seralize,
                                                                  HPointLightEntity::Deseralize,
                                                                  HPointLightEntity::CreateEntity });
        m_serializer.RegisterAClass(crc32("HImageBasedLightingEntity"), { HImageBasedLightingEntity::Seralize,
                                                                          HImageBasedLightingEntity::Deseralize,
                                                                          HImageBasedLightingEntity::CreateEntity });
        m_serializer.RegisterAClass(crc32("HBackgroundCubemapEntity"), { HBackgroundCubemapEntity::Seralize,
                                                                         HBackgroundCubemapEntity::Deseralize,
                                                                         HBackgroundCubemapEntity::CreateEntity });
        // Register custom entity classes
        RegisterCustomSerializeClass();
    }

    // ================================================================================================================
    void HFrameListener::EntitiesPreRenderTick()
    {
        HScene& activeScene = GetActiveScene();
        std::chrono::time_point<std::chrono::high_resolution_clock> currentTimeStamp = std::chrono::high_resolution_clock::now();

        if (m_elapsedSec < 0.0)
        {
            m_elapsedSec = 0.0;
        }
        else
        {
            std::chrono::duration<double> elapsed_seconds = currentTimeStamp - m_lastTimeStamp;
            m_elapsedSec = elapsed_seconds.count();
        }
        
        activeScene.PreRenderTick(m_elapsedSec);
        m_lastTimeStamp = currentTimeStamp;
    }

    // ================================================================================================================
    void HFrameListener::EntitiesPostRenderTick()
    {
        HScene& activeScene = GetActiveScene();
        activeScene.PostRenderTick(m_elapsedSec);
    }
}