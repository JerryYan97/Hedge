#include "HEvent.h"
#include "../scene/HScene.h"
#include "HEntity.h"
#include "Utils.h"

namespace Hedge
{
    // ================================================================================================================
    HEvent::HEvent(
        const HEventArguments& arg, 
        const std::string& type)
        : m_isHandled(false)
    {
        m_typeHash = crc32(type.data());
        m_arg = arg;
    }

    // ================================================================================================================
    void HEventManager::RegisterListener(
        const std::string& type, 
        const uint32_t     entity)
    {
        size_t typeHash = crc32(type.data());

        if (m_eventListenerMap.find(typeHash) != m_eventListenerMap.end())
        {
            // There are listeners for the target event type. Add the entity handle into the listener list.
            m_eventListenerMap[typeHash].push_back(entity);
        }
        else
        {
            // No listener for the target event type. Create the listener list and add the entity into it.
            m_eventListenerMap[typeHash] = std::list<uint32_t>();
            m_eventListenerMap[typeHash].push_back(entity);
        }
    }

    // ================================================================================================================
    void HEventManager::SendEvent(
        HEvent& hEvent, 
        HScene* pScene)
    {
        if (m_eventListenerMap.find(hEvent.GetEventType()) != m_eventListenerMap.end())
        {
            std::list<uint32_t>& list = m_eventListenerMap[hEvent.GetEventType()];
            for (auto& itr : list)
            {
                pScene->GetEntity(itr)->OnEvent(hEvent);
            }
        }
    }
}