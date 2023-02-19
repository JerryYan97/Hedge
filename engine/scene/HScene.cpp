#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"

namespace Hedge
{
    // ================================================================================================================
    HScene::HScene()
    {}

    // ================================================================================================================
    HScene::~HScene()
    {
        for (auto p : m_entitiesHashTable)
        {
            delete p.second;
        }
    }

    // ================================================================================================================
    void HScene::SpawnEntity(HEntity* pEntity)
    {
        entt::entity newEntity = m_registry.create();
        uint32_t entityHandle = static_cast<uint32_t>(newEntity);
        pEntity->CreateInSceneInternal(this, entityHandle);

        pEntity->OnDefineEntity();
        
        m_entitiesHashTable.insert({ entityHandle, pEntity });
    }

    // ================================================================================================================
    SceneRenderInfo HScene::GetSceneRenderInfo()
    {
        SceneRenderInfo renderInfo{};

        auto staticMeshView = m_registry.view<StaticMeshComponent>();
        for (auto entity : staticMeshView)
        {
            auto& meshComponent = staticMeshView.get<StaticMeshComponent>(entity);
            renderInfo.m_pIdx = meshComponent.m_pIdx;
            renderInfo.m_pVert = meshComponent.m_pVert;
            renderInfo.m_vertCnt = meshComponent.m_vertCnt;
        }

        return renderInfo;
    }
}