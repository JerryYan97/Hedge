#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"
#include "../util/UtilMath.h"

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
    void HScene::SpawnEntity(
        HEntity* pEntity, 
        HEventManager& eventManager)
    {
        entt::entity newEntity = m_registry.create();
        uint32_t entityHandle = static_cast<uint32_t>(newEntity);
        pEntity->CreateInSceneInternal(this, entityHandle);

        pEntity->OnDefineEntity(eventManager);
        
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
            auto& transComponent = m_registry.get<TransformComponent>(entity);

            renderInfo.m_pIdx = meshComponent.m_pIdx;
            renderInfo.m_pVert = meshComponent.m_pVert;
            renderInfo.m_vertCnt = meshComponent.m_vertCnt;
            renderInfo.m_vertBufBytes = meshComponent.m_vertBufBytes;

            GenModelMat(transComponent.m_pos,
                        transComponent.m_rot[2],
                        transComponent.m_rot[0],
                        transComponent.m_rot[1],
                        transComponent.m_scale,
                        renderInfo.m_modelMat);
        }

        auto cameraEntityView = m_registry.view<CameraComponent>();
        for (auto entity : cameraEntityView)
        {
            auto& camComponent = cameraEntityView.get<CameraComponent>(entity);
            auto& transComponent = m_registry.get<TransformComponent>(entity);

            float viewMat[16] = {};
            float persMat[16] = {};
            GenViewMatUpdateUp(camComponent.m_view, transComponent.m_pos, camComponent.m_up, viewMat);
            GenPerspectiveProjMat(camComponent.m_near, camComponent.m_far, camComponent.m_fov, camComponent.m_aspect, persMat);
            MatrixMul4x4(persMat, viewMat, renderInfo.m_vpMat);
        }

        return renderInfo;
    }
}