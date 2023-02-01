#pragma once
#include <entt.hpp>
#include <unordered_map>

namespace Hedge
{
    class HEntity;

    class HScene
    {
    public:
        HScene();
        ~HScene();

        void SpawnEntity(HEntity* pEntity);

        template<typename... Args>
        void EntityAddComponent(uint32_t entityHandle, Args &&...args);
        
        template<typename T>
        T& EntityGetComponent(uint32_t entityHandle);
        
        
    private:
        entt::registry m_registry;
        std::unordered_map<uint32_t, HEntity*> m_entitiesHashTable;
    };
}