#pragma once
#include <entt.hpp>
#include <unordered_map>
#include "../core/HEntity.h"

namespace Hedge
{
    class HEntity;

    class HScene
    {
    public:
        HScene();
        ~HScene();

        void SpawnEntity(HEntity& entity);
        
    private:
        entt::registry m_registry;
        std::unordered_map<uint32_t, HEntity*> m_entitiesHashTable;
    };
}