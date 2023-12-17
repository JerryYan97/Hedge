#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"
#include "../util/UtilMath.h"
#include "../core/HAssetRsrcManager.h"

extern Hedge::HAssetRsrcManager* g_pAssetRsrcManager;

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

            HStaticMeshAsset* pStaticMeshAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(meshComponent.m_meshAssetGuid, (HAsset**)&pStaticMeshAsset);

            renderInfo.objsIdxBuffers.push_back(pStaticMeshAsset->GetIdxGpuBuffer(0));
            renderInfo.idxCounts.push_back(pStaticMeshAsset->GetIdxCnt(0));

            renderInfo.objsVertBuffers.push_back(pStaticMeshAsset->GetVertGpuBuffer(0));
            renderInfo.vertCounts.push_back(pStaticMeshAsset->GetVertCnt(0));

            HMaterialAsset* pMaterialAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(pStaticMeshAsset->GetMaterialGUID(0), (HAsset**)&pMaterialAsset);
            
            HTextureAsset* pBaseColorTextureAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(pMaterialAsset->GetBaseColorTextureGUID(), (HAsset**)&pBaseColorTextureAsset);
            renderInfo.modelBaseColors.push_back(pBaseColorTextureAsset->GetGpuImgPtr());

            HTextureAsset* pNormalMapAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(pMaterialAsset->GetNormalMapGUID(), (HAsset**)&pNormalMapAsset);
            renderInfo.modelNormalTexs.push_back(pNormalMapAsset->GetGpuImgPtr());

            HTextureAsset* pMetallicRoughnessTextureAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(pMaterialAsset->GetMetallicRoughnessGUID(), (HAsset**)&pMetallicRoughnessTextureAsset);
            renderInfo.modelMetallicRoughnessTexs.push_back(pMetallicRoughnessTextureAsset->GetGpuImgPtr());

            HTextureAsset* pOcclusionAsset = nullptr;
            g_pAssetRsrcManager->GetAssetPtr(pMaterialAsset->GetOcclusionGUID(), (HAsset**)&pOcclusionAsset);
            renderInfo.modelOcclusionTexs.push_back(pOcclusionAsset->GetGpuImgPtr());

            HMat4x4 modelMat{};
            GenModelMat(transComponent.m_pos,
                        transComponent.m_rot[2],
                        transComponent.m_rot[0],
                        transComponent.m_rot[1],
                        transComponent.m_scale,
                        modelMat.eles);
            renderInfo.modelMats.push_back(modelMat);
        }

        // TODO: Need to have an active camera check.
        auto cameraEntityView = m_registry.view<CameraComponent>();
        for (auto entity : cameraEntityView)
        {
            auto& camComponent = cameraEntityView.get<CameraComponent>(entity);
            auto& transComponent = m_registry.get<TransformComponent>(entity);

            float viewMat[16] = {};
            float persMat[16] = {};
            GenViewMat(camComponent.m_view, transComponent.m_pos, camComponent.m_up, viewMat);
            GenPerspectiveProjMat(camComponent.m_near, camComponent.m_far, camComponent.m_fov, camComponent.m_aspect, persMat);
            MatrixMul4x4(persMat, viewMat, renderInfo.vpMat.eles);
        }

        auto pointLightsView = m_registry.view<PointLightComponent>();
        for (auto entity : pointLightsView)
        {
            auto& pointLightComponent = pointLightsView.get<PointLightComponent>(entity);
            auto& transComponent = m_registry.get<TransformComponent>(entity);

            HVec3 pos;
            memcpy(&pos, transComponent.m_pos, sizeof(transComponent.m_pos));

            HVec3 radiance;
            memcpy(&radiance, pointLightComponent.m_color, sizeof(pointLightComponent.m_color));

            renderInfo.pointLightsPositions.push_back(pos);
            renderInfo.pointLightsRadiances.push_back(radiance);
        }

        return renderInfo;
    }
}