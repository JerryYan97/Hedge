#include "HAssetRsrcManager.h"
#include "g_builtInModels.h"
#include "Utils.h"
#include "yaml-cpp/yaml.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Hedge
{
    // ================================================================================================================
    std::string GetNamePathFolderName(
        const std::string& assetNamePath)
    {
        size_t idx = assetNamePath.rfind('/');
        return assetNamePath.substr(idx);
    }

    // ================================================================================================================
    HAssetRsrcManager::HAssetRsrcManager()
    {
    }

    // ================================================================================================================
    HAssetRsrcManager::~HAssetRsrcManager()
    {
        CleanAllAssets();
    }

    // ================================================================================================================

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadAsset(
        const std::string& assetNamePath)
    {
        uint64_t guid = crc32(assetNamePath.c_str());
        if (m_assetsMap.count(guid) > 0)
        {
            m_assetsMap[guid].refCounter++;
        }
        else
        {
            // TODO: We may need a better design for loading assets, but the current naive design won't cause big
            //       troubles in the future.
            AssetWrap assetWrap;
            assetWrap.refCounter = 1;

            // Read the type of the asset
            std::string assetConfigFileNamePath = assetNamePath + "/" + GetNamePathFolderName(assetNamePath) + ".yml";
            YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());
            
            std::string assetTypeStr = config["asset type"].as<std::string>();
            if (crc32(assetTypeStr.c_str()) == crc32("HStaticMeshAsset"))
            {
                assetWrap.pAsset = new HStaticMeshAsset(guid, assetNamePath, this);
            }
            else if (crc32(assetTypeStr.c_str()) == crc32("HMaterialAsset"))
            {
                assetWrap.pAsset = new HMaterialAsset(guid, assetNamePath, this);
            }
            else if (crc32(assetTypeStr.c_str()) == crc32("HTextureAsset"))
            {
                assetWrap.pAsset = new HTextureAsset(guid, assetNamePath, this);
            }
            else
            {
                assert(1, "Unrecognized asset type.");
            }

            assetWrap.pAsset->LoadAssetFromDisk();
            m_assetsMap.insert({ guid, assetWrap });
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
        uint64_t           guid,
        std::string        assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        m_guid(guid),
        m_assetPathName(assetPathName),
        m_pAssetRsrcManager(pAssetRsrcManager)
    {
    }

    // ================================================================================================================
    HStaticMeshAsset::HStaticMeshAsset(
        uint64_t           guid,
        std::string        assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_materialGUID(0),
        m_materialPathName("None")
    {
    }

    // ================================================================================================================
    void HStaticMeshAsset::LoadAssetFromDisk()
    {
        // Read the configuration file
        std::string assetConfigFileNamePath = m_assetPathName + "/" + GetNamePathFolderName(m_assetPathName) + ".yml";
        YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());
         
        // Load other assets according to the configuation file
        // E.g. The material asset on this static mesh asset.


        // Load the raw geometry

    }

    // ================================================================================================================
    HTextureAsset::HTextureAsset(
        uint64_t    guid,
        std::string assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_widthPix(0),
        m_heightPix(0),
        m_elePerPix(0),
        m_bytesPerEle(0)
    {}

    // ================================================================================================================
    void HTextureAsset::LoadAssetFromDisk()
    {

    }

    // ================================================================================================================
    // Material doesn't have a src file.
    HMaterialAsset::HMaterialAsset(
        uint64_t    guid,
        std::string assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager)
    {}
}