#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace Hedge
{
    class HAssetRsrcManager;

    // All assets have their own path name in the game or in the game project.
    // They can be stored on the disk and loaded from the disk.
    // On the disk, all assets are in the 'assets' folder of the game or game project.
    // On the disk, the asset should be put into a folder with the m_assetPathName and a same name yaml file.
    class HAsset
    {
    public:
        HAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HAsset() {}

        virtual void LoadAssetFromDisk() = 0;

    protected:
        HAssetRsrcManager* m_pAssetRsrcManager;
        const std::string  m_assetPathName;

    private:
        const uint64_t    m_guid;
    };

    // Static mesh has raw geometry data and a material.
    class HStaticMeshAsset : public HAsset
    {
    public:
        HStaticMeshAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);

        ~HStaticMeshAsset() { }

        virtual void LoadAssetFromDisk();

    private:
        std::string m_materialPathName;
        uint64_t    m_materialGUID;

        std::vector<uint16_t> m_idxData;
        std::vector<float>    m_vertData;
    };

    // A material only describes the property of a mesh surface.
    // It can have multiple textures.
    class HMaterialAsset : public HAsset
    {
    public:
        HMaterialAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HMaterialAsset() {};

        virtual void LoadAssetFromDisk();

    private:
    };

    // A texuture asset holds an image data.
    class HTextureAsset : public HAsset
    {
    public:
        HTextureAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HTextureAsset() {}

        virtual void LoadAssetFromDisk();

    private:
        uint32_t             m_widthPix;
        uint32_t             m_heightPix;
        uint8_t              m_elePerPix;
        uint8_t              m_bytesPerEle;
        std::vector<float>   m_dataFloat;
        std::vector<uint8_t> m_dataUInt8;
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