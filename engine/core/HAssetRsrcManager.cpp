#include "HAssetRsrcManager.h"
#include "g_builtInModels.h"
#include "Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Hedge
{
    // ================================================================================================================
    HAssetRsrcManager::HAssetRsrcManager()
    {
        // Load built in meshes. Built in meshes are always in the RAM until the game ends.
        // Cube:
        {
            uint64_t builtInCubeId = crc32(CubeStrId.c_str());
            HStaticMeshAsset* pBuiltInCube = new HStaticMeshAsset(builtInCubeId, CubeStrId, "BUILT_IN");
            AssetWrap builtInCubeWrap;
            builtInCubeWrap.refCounter = 1;
            builtInCubeWrap.pAsset = pBuiltInCube;
            m_assetsMap.insert({ builtInCubeId, builtInCubeWrap });
        }
    }

    // ================================================================================================================
    HAssetRsrcManager::~HAssetRsrcManager()
    {
        CleanAllAssets();
    }

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadAsset(
        const std::string& objNamePath)
    {
        uint64_t guid = crc32(objNamePath.c_str());
        if (m_assetsMap.count(guid) > 0)
        {
            m_assetsMap[guid].refCounter++;
        }
        else
        {
            AssetWrap assetWrap;
            assetWrap.refCounter = 1;
        }

        return guid;
    }

    // ================================================================================================================
    bool HAssetRsrcManager::GetAssetPtr(
        uint64_t guid,
        HAsset** pPtr)
    {
        if (m_assetsMap.count(guid) > 0)
        {
            *pPtr = m_assetsMap.at(guid).pAsset;
            return true;
        }
        else
        {
            *pPtr = nullptr;
            return false;
        }
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
                void* ptr = m_assetsMap.at(guid).pAsset;
                delete ptr;
                m_assetsMap.erase(guid);
            }
        }
    }

    // ================================================================================================================
    void HAssetRsrcManager::CleanAllAssets()
    {
        for (auto itr : m_assetsMap)
        {
            delete itr.second.pAsset;
        }
        m_assetsMap.clear();
    }

    // ================================================================================================================
    HAsset::HAsset(
        uint64_t    guid,
        std::string assetPathName,
        std::string srcFile)
        : m_guid(guid),
          m_assetPathName(assetPathName),
          m_sourceFile(srcFile)
    {}

    // ================================================================================================================
    HStaticMeshAsset::HStaticMeshAsset(
        uint64_t    guid,
        std::string assetPathName,
        std::string srcFile)
        : HAsset(guid, assetPathName, srcFile)
    {}

    // ================================================================================================================
    HTextureAsset::HTextureAsset(
        uint64_t    guid,
        std::string assetPathName,
        std::string srcFile)
        : HAsset(guid, assetPathName, srcFile)
    {}

    // ================================================================================================================
    // Material doesn't have a src file.
    HMaterialAsset::HMaterialAsset(
        uint64_t    guid,
        std::string assetPathName)
        : HAsset(guid, assetPathName, "None")
    {}
}