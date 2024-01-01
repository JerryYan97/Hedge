#pragma once
#include <entt.hpp>
#include <unordered_map>
#include <vector>
#include "../core/HGpuRsrcManager.h"

namespace Hedge
{
    class HEntity;
    class HEventManager;

    struct HMat4x4
    {
        float eles[16];
    };

    struct HVec3
    {
        float eles[3];
    };

    // TODO: We may need a large one batch vertex + idx buffer.
    struct SceneRenderInfo
    {
        std::vector<HGpuBuffer*> objsIdxBuffers;
        std::vector<uint32_t>   idxCounts;
        
        std::vector<HGpuBuffer*> objsVertBuffers;
        std::vector<uint32_t>    vertCounts;
        
        std::vector<uint64_t> objsMaterialsGuid;
        
        std::vector<HMat4x4>  modelMats;
        std::vector<HGpuImg*> modelBaseColors;
        std::vector<HGpuImg*> modelNormalTexs;
        std::vector<HGpuImg*> modelMetallicRoughnessTexs;
        std::vector<HGpuImg*> modelOcclusionTexs;

        std::vector<HVec3> pointLightsPositions;
        std::vector<HVec3> pointLightsRadiances;

        // Image based lightning
        HGpuImg* diffuseCubemapGpuImg;
        HGpuImg* prefilterEnvCubemapGpuImg;
        HGpuImg* envBrdfGpuImg;
        
        float iblMaxMipLevels;

        HMat4x4 vpMat;
        float   cameraPos[3];
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
        void CreateDummyBlackTextures();

        entt::registry m_registry;
        std::unordered_map<uint32_t, HEntity*> m_entitiesHashTable;

        HGpuImg* m_pDummyBlackCubemap;
        HGpuImg* m_pDummyBlack2dImg;
    };
}