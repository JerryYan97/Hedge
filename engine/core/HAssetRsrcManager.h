#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace Hedge
{
    // All assets have their own path name in the game or in the game project.
    // They can be stored on the disk or load from the disk.
    class Asset
    {
    public:
        Asset() {}
        ~Asset() {}
    };

    class StaticMeshAsset : public Asset
    {
    public:
        StaticMeshAsset() : Asset() {}
        ~StaticMeshAsset() { }
    };

    struct MeshData
    {
        std::vector<uint16_t> idxData;
        std::vector<float>    vertData;
    };

    struct ImgData
    {
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
        uint64_t LoadObjToMeshData(const std::string& objNamePath);
        uint64_t LoadStaticCubeMesh();
        uint64_t LoadImg(const std::string& imgNamePath);

        void ReleaseAsset(uint64_t guid);

        // The reference count is not tracked in the 'GetAssetPtr' function.
        bool GetAssetPtr(uint64_t guid, void** pPtr);

    protected:

    private:
        // void UnloadAsset(uint64_t guid);
        void CleanAllAssets();

        struct AssetWrap
        {
            void*    pRes;
            uint32_t refCounter;
        };
        std::unordered_map<uint64_t, AssetWrap> m_assetsMap;

    };
}