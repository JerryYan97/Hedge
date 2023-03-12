#pragma once
#include <entt.hpp>
#include <unordered_map>

namespace Hedge
{
    class HEntity;
    class HEventManager;

    struct SceneRenderInfo
    {
        uint32_t* m_pIdx;
        float*    m_pVert;
        uint32_t  m_idxNum;
        uint32_t  m_vertBufBytes;
        float m_modelMat[16];
        float m_vpMat[16];
    };

    class HScene
    {
    public:
        HScene();
        ~HScene();

        // Entity functions
        void SpawnEntity(HEntity* pEntity, HEventManager& eventManager);

        HEntity* GetEntity(uint32_t entityHandle) { return m_entitiesHashTable[entityHandle]; }

        // Component functions
        template<typename Type, typename... Args>
        void EntityAddComponent(uint32_t entityHandle, Args &&...args) 
            { m_registry.emplace<Type>(static_cast<entt::entity>(entityHandle), std::forward<Args>(args)...); }
        
        template<typename T>
        T& EntityGetComponent(uint32_t entityHandle)
            { return m_registry.get<T>(static_cast<entt::entity>(entityHandle)); }
        
        SceneRenderInfo GetSceneRenderInfo();

        std::unordered_map<uint32_t, HEntity*>& GetEntityHashTable() { return m_entitiesHashTable; }

        bool IsEmpty() { return m_entitiesHashTable.empty(); }
        
    private:
        entt::registry m_registry;
        std::unordered_map<uint32_t, HEntity*> m_entitiesHashTable;
    };
}