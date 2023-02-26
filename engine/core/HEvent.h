#pragma once
#include <string>
#include <unordered_map>
#include <any>
#include <list>

// TODO: We may need a compile-time string hash system. The MSVC's std::hash<> cannot be used as the hash function 
// during compile-time since it is not a constexpr function. We maybe able to limit the length of the string.
// Ref: https://cp-algorithms.com/string/string-hashing.html

/*
*  Some predefined events strings:
*  IO category: MOUSE_MIDDLE_BUTTON, KEY_W, KEY_S
* 
*  MOUSE_MIDDLE_BUTTON: IS_DOWN (bool), POS (HFVec2)
*  KEY_W: IS_DOWN (bool)
*/

namespace Hedge
{
    typedef std::unordered_map<size_t, std::any> HEventArguments; // Argument name -- Argument value

    class HScene;

    class HEvent 
    {
    public:
        explicit HEvent(const HEventArguments& arg, const std::string& type);
        ~HEvent() {};

        size_t GetEventType() const { return m_typeHash; }
        HEventArguments& GetArgs() { return m_arg; }

    private:
        HEventArguments m_arg;
        size_t m_typeHash;
        bool m_isHandled;
    };

    // Manage interested objects listening and registration.
    class HEventManager
    {
    public:
        HEventManager()
            : m_eventListenerMap()
        {};
        ~HEventManager() {};

        void RegisterListener(const std::string& type, const uint32_t entity);
        void SendEvent(HEvent& hEvent, HScene* pScene);

    private:
        std::unordered_map<size_t, std::list<uint32_t>> m_eventListenerMap; // Event type -- Linked list of registered objects.
    };
}