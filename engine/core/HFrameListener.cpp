#include "HFrameListener.h"
#include "HEntity.h"
#include "Utils.h"

namespace Hedge
{
    // ================================================================================================================
    HFrameListener::HFrameListener()
        :m_eventManager()
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

        // Register custom entity classes
        RegisterCustomSerializeClass();
    }
}