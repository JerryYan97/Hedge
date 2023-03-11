#include "HFrameListener.h"

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
        // Register all engine classes

        // Register custom classes
        RegisterCustomSerializeClass();
    }
}