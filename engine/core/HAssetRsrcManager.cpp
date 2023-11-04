#include "HAssetRsrcManager.h"
#include "g_builtInModels.h"
#include "Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Hedge
{
    // ================================================================================================================
    HAssetRsrcManager::HAssetRsrcManager()
    {}

    // ================================================================================================================
    HAssetRsrcManager::~HAssetRsrcManager()
    {
        CleanAllAssets();
    }

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadObjToMeshData(
        const std::string& objNamePath)
    {
        uint64_t guid = crc32(objNamePath.c_str());
        if (m_assetsMap.count(guid) > 0)
        {
            
        }
        else
        {
            
        }

        m_assetsMap[guid].refCounter++;
        return guid;
    }

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadStaticCubeMesh()
    {
        const std::string staticCubeMeshStrId = "BuiltIn_Static_Cube_Mesh";
        uint64_t guid = crc32(staticCubeMeshStrId.c_str());
        
        if (m_assetsMap.count(guid) == 0)
        {
            MeshData* pMeshData = new MeshData();
            memset(pMeshData, 0, sizeof(MeshData));
            memcpy(pMeshData->idxData.data(), CubeIdxData, sizeof(CubeIdxData));
            memcpy(pMeshData->vertData.data(), CubeVertBufData, sizeof(CubeVertBufData));

            AssetWrap assetWrap;
            assetWrap.pRes = pMeshData;
            assetWrap.refCounter = 1;

            m_assetsMap.insert({ guid, assetWrap });
        }

        return guid;
    }

    // ================================================================================================================
    bool HAssetRsrcManager::GetAssetPtr(
        uint64_t guid,
        void**   pPtr)
    {
        if (m_assetsMap.count(guid) > 0)
        {
            *pPtr = m_assetsMap.at(guid).pRes;
            return true;
        }
        else
        {
            *pPtr = nullptr;
            return false;
        }
    }

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadImg(
        const std::string& imgNamePath)
    {
        uint64_t guid = crc32(imgNamePath.c_str());
        if (m_assetsMap.count(guid) > 0)
        {
            
        }
        else
        {

        }
        return guid;
    }

    // ================================================================================================================
    void HAssetRsrcManager::ReleaseAsset(
        uint64_t guid)
    {
        if (m_assetsMap.count(guid) > 0)
        {
            m_assetsMap[guid].refCounter--;
            if (m_assetsMap[guid].refCounter == 0)
            {
                void* ptr = m_assetsMap.at(guid).pRes;
                delete ptr;
                m_assetsMap.erase(guid);
            }
        }
    }

    // ================================================================================================================
    /*
    void HAssetRsrcManager::UnloadAsset(
        uint64_t guid)
    {
        if (m_assetsMap.count(guid) > 0)
        {
            void* ptr = m_assetsMap.at(guid).pRes;
            delete ptr;
            m_assetsMap.erase(guid);
        }
    }
    */

    // ================================================================================================================
    void HAssetRsrcManager::CleanAllAssets()
    {
        for (auto itr : m_assetsMap)
        {
            delete itr.second.pRes;
        }
        m_assetsMap.clear();
    }
}