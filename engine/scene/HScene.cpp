#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"

namespace Hedge
{
    // ================================================================================================================
    HScene::HScene()
        : m_reuseRenderScene(false)
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

        if (m_reuseRenderScene == false)
        {
            auto view = m_registry.view<StaticMeshComponent>();
            for (auto entity : view)
            {
                auto& meshComponent = view.get<StaticMeshComponent>(entity);
                renderInfo.m_pIdx = meshComponent.m_pIdx;
                renderInfo.m_pVert = meshComponent.m_pVert;
                renderInfo.m_vertCnt = meshComponent.m_vertCnt;
                renderInfo.m_reuse = false;

                m_reuseRenderScene = true;
            }
        }
        else
        {
            renderInfo.m_reuse = true;
        }

        return renderInfo;
    }
}