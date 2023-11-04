#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace Hedge
{
    // All assets have their own path name in the game or in the game project.
    // They can be stored on the disk or load from the disk.
    // On the disk, all assets are in the 'assets' folder of the game or game project.
    class HAsset
    {
    public:
        HAsset(uint64_t guid, std::string assetPathName, std::string srcFile);
        ~HAsset() {}

    protected:


    private:
        const uint64_t    m_guid;
        const std::string m_assetPathName;
        const std::string m_sourceFile;
    };

    class HStaticMeshAsset : public HAsset
    {
    public:
        HStaticMeshAsset(uint64_t guid, std::string assetPathName, std::string srcFile);

        ~HStaticMeshAsset() { }

    private:
        std::vector<uint16_t> m_idxData;
        std::vector<float>    m_vertData;
    };

    class HMaterialAsset : public HAsset
    {
    public:
        HMaterialAsset(uint64_t guid, std::string assetPathName);
        ~HMaterialAsset() {};

    private:
    };

    class HTextureAsset : public HAsset
    {
    public:
        HTextureAsset(uint64_t guid, std::string assetPathName, std::string srcFile);
        ~HTextureAsset() {}

    private:
        uint32_t             widthPix;
        uint32_t             heightPix;
        uint8_t              elePerPix;
        uint8_t              bytesPerEle;
        std::vector<float>   dataFloat;
        std::vector<uint8_t> dataUInt8;
    };

    // GUID - RAM ptr based asset manager.
    // GUID is generated from an asset's path name so it can be used to check whether the rsrc has been loaded into the system.
    // For built in static meshes or data, we need to assign them unique strings.
    class HAssetRsrcManager
    {
    public:
        HAssetRsrcManager();
        ~HAssetRsrcManager();

        // We track the reference counter in Loadxxx or ReleaseAsset function.
        // The resource is unloaded when it's reference count become 0.
        uint64_t LoadAsset(const std::string& assetNamePath);
        void ReleaseAsset(uint64_t guid);

        // The reference count is not tracked in the 'GetAssetPtr' function.
        bool GetAssetPtr(uint64_t guid, HAsset** pPtr);

    protected:

    private:
        void CleanAllAssets();

        struct AssetWrap
        {
            HAsset*  pAsset;
            uint32_t refCounter;
        };
        std::unordered_map<uint64_t, AssetWrap> m_assetsMap;

    };
}