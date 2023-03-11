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
        m_serializer.RegisterAClass(crc32("HCubeEntity"), HCubeEntity::Deseralize);
        m_serializer.RegisterAClass(crc32("HCameraEntity"), HCameraEntity::Deseralize);
        m_serializer.RegisterAClass(crc32("HPointLightEntity"), HPointLightEntity::Deseralize);

        // Register custom entity classes
        RegisterCustomSerializeClass();
    }
}