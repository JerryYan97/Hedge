#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"
#include "../util/UtilMath.h"
#include "../core/HAssetRsrcManager.h"

extern Hedge::HAssetRsrcManager* g_pAssetRsrcManager;
extern Hedge::HGpuRsrcManager* g_pGpuRsrcManager;

namespace Hedge
{
    // ================================================================================================================
    HScene::HScene() :
        m_pDummyBlackCubemap(nullptr)
    {}

    // ================================================================================================================
    HScene::~HScene()
    {
        for (auto p : m_entitiesHashTable)
        {
            delete p.second;
        }

        if (m_pDummyBlackCubemap)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pDummyBlackCubemap);
        }

        if (m_pDummyBlack2dImg)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pDummyBlack2dImg);
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
    void HScene::CreateDummyBlackTextures()
    {
        // Cubemap
        {
            VkImageSubresourceRange imgSubRsrcRange{};
            {
                imgSubRsrcRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imgSubRsrcRange.baseArrayLayer = 0;
                imgSubRsrcRange.layerCount = 6;
                imgSubRsrcRange.baseMipLevel = 0;
                imgSubRsrcRange.levelCount = 1;
            }

            VkSamplerCreateInfo samplerInfo{};
            {
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.minLod = -1000;
                samplerInfo.maxLod = 1000;
                samplerInfo.maxAnisotropy = 1.0f;
            }

            HGpuImgCreateInfo dummyBlackCubemapInfo{};
            {
                dummyBlackCubemapInfo.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                dummyBlackCubemapInfo.hasSampler = true;
                dummyBlackCubemapInfo.imgSubresRange = imgSubRsrcRange;
                dummyBlackCubemapInfo.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                dummyBlackCubemapInfo.imgViewType = VK_IMAGE_VIEW_TYPE_CUBE;
                dummyBlackCubemapInfo.samplerInfo = samplerInfo;
                dummyBlackCubemapInfo.imgCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                dummyBlackCubemapInfo.imgExtent = VkExtent3D{ 1, 1, 1 };
                dummyBlackCubemapInfo.imgFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
            }

            m_pDummyBlackCubemap = g_pGpuRsrcManager->CreateGpuImage(dummyBlackCubemapInfo, "m_pDummyBlackCubemap");

            VkClearColorValue clearColorVal{ 0.f, 0.f, 0.f, 1.f };
            g_pGpuRsrcManager->CleanColorGpuImage(m_pDummyBlackCubemap, &clearColorVal);

            g_pGpuRsrcManager->TransImageLayout(m_pDummyBlackCubemap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        // 2D black texture
        {
            VkImageSubresourceRange imgSubRsrcRange{};
            {
                imgSubRsrcRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imgSubRsrcRange.baseArrayLayer = 0;
                imgSubRsrcRange.layerCount = 1;
                imgSubRsrcRange.baseMipLevel = 0;
                imgSubRsrcRange.levelCount = 1;
            }

            VkSamplerCreateInfo samplerInfo{};
            {
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.minLod = -1000;
                samplerInfo.maxLod = 1000;
                samplerInfo.maxAnisotropy = 1.0f;
            }

            HGpuImgCreateInfo dummyBlack2dInfo{};
            {
                dummyBlack2dInfo.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                dummyBlack2dInfo.hasSampler = true;
                dummyBlack2dInfo.imgSubresRange = imgSubRsrcRange;
                dummyBlack2dInfo.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                dummyBlack2dInfo.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
                dummyBlack2dInfo.samplerInfo = samplerInfo;
                dummyBlack2dInfo.imgExtent = VkExtent3D{ 1, 1, 1 };
                dummyBlack2dInfo.imgFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
            }

            m_pDummyBlack2dImg = g_pGpuRsrcManager->CreateGpuImage(dummyBlack2dInfo, "m_pDummyBlack2dImg");

            VkClearColorValue clearColorVal{ 0.f, 0.f, 0.f, 1.f };
            g_pGpuRsrcManager->CleanColorGpuImage(m_pDummyBlack2dImg, &clearColorVal);

            g_pGpuRsrcManager->TransImageLayout(m_pDummyBlack2dImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

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

        // Check whether the scene has IBL. If we don't have, then we need to use black texture asset to init IBL
        // textures
        auto iblView = m_registry.view<HImageBasedLightingEntity>();
        if (iblView.empty())
        {
            if ((m_pDummyBlackCubemap == nullptr) || (m_pDummyBlack2dImg == nullptr))
            {
                CreateDummyBlackTextures();
            }

            renderInfo.diffuseCubemapGpuImg = m_pDummyBlackCubemap;
            renderInfo.prefilterEnvCubemapGpuImg = m_pDummyBlackCubemap;
            renderInfo.envBrdfGpuImg = m_pDummyBlack2dImg;
        }

        return renderInfo;
    }
}