#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

// NOTE: For the gpu rsrc of each assets, we just store them along with their RAM data. However, the gpu rsrc doesn't
//       have any guids but in reality they can also have guid like any assets, since they can have unique names and it
//       also enables the gpu rsrc share between different gpu rsrc.

namespace Hedge
{
    class HAssetRsrcManager;
    struct HGpuBuffer;
    struct HGpuImg;

    // All assets have their own path name in the game or in the game project.
    // They can be stored on the disk and loaded from the disk.
    // On the disk, all assets are in the 'assets' folder of the game or game project.
    // On the disk, the asset should be put into a folder with the m_assetPathName and a same name yaml file.
    class HAsset
    {
    public:
        // The input asset path name should be absolute.
        HAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HAsset() {}

        virtual void LoadAssetFromDisk() = 0;

    protected:
        HAssetRsrcManager* m_pAssetRsrcManager;
        const std::string  m_assetPathName;      // Absolute path name.

    private:
        const uint64_t    m_guid;
    };

    struct Mesh
    {
        float modelPos[4];
        std::vector<float>    vertData;
        std::vector<uint16_t> idxData;

        HGpuBuffer* pIdxDataGpuBuffer;
        HGpuBuffer* pVertDataGpuBuffer;

        std::string materialPathName;
        uint64_t    materialGUID;
    };

    // Static mesh has raw geometry data and a material.
    // Note that for gltf models, we may need to import them first before we can use them.
    // They may have lots of sub-models that we need to generate the custom asset format first.
    class HStaticMeshAsset : public HAsset
    {
    public:
        HStaticMeshAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);

        ~HStaticMeshAsset();

        virtual void LoadAssetFromDisk() override;

        uint32_t GetSectionCounts() { return m_meshes.size(); }

        HGpuBuffer* GetIdxGpuBuffer(uint32_t i);
        HGpuBuffer* GetVertGpuBuffer(uint32_t i);
        uint64_t GetMaterialGUID(uint32_t i) { return m_meshes[i].materialGUID; }
        uint32_t GetIdxCnt(uint32_t i) { return m_meshes[i].idxData.size(); }
        uint32_t GetVertCnt(uint32_t i) { return m_meshes[i].vertData.size() / 12; }

    private:
        void LoadGltfRawGeo(const std::string& namePath);

        std::vector<Mesh> m_meshes;

        // Note: for a model, it's possible that it has multiple sections or sub-models.
        //       (Helmet's glass, top and mouth cover, etc)
        // std::vector<std::string> m_materialPathNames;
        // std::vector<uint64_t>    m_materialGUIDs;

        // std::vector<std::vector<uint16_t>> m_idxDataVec;
        // std::vector<std::vector<float>>    m_vertDataVec;

        // std::vector<HGpuBuffer*> m_pIdxDataGpuBuffers;
        // std::vector<HGpuBuffer*> m_pVertDataGpuBuffers;
    };

    // A material only describes the property of a mesh surface.
    // It can have multiple textures.
    class HMaterialAsset : public HAsset
    {
    public:
        HMaterialAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HMaterialAsset() {};

        virtual void LoadAssetFromDisk() override;

        uint64_t GetBaseColorTextureGUID() { return m_baseColorTextureGUID; }
        uint64_t GetNormalMapGUID() { return m_normalMapGUID; }
        uint64_t GetMetallicRoughnessGUID() { return m_metallicRoughnessGUID; }
        uint64_t GetOcclusionGUID() { return m_occlusionGUID; }

    private:
        std::string m_baseColorTexturePathName;
        uint64_t    m_baseColorTextureGUID;

        std::string m_normalMapPathName;
        uint64_t    m_normalMapGUID;

        std::string m_metallicRoughnessPathName;
        uint64_t    m_metallicRoughnessGUID;

        std::string m_occlusionPathName;
        uint64_t    m_occlusionGUID;
    };

    // A texuture asset holds an image data.
    class HTextureAsset : public HAsset
    {
    public:
        HTextureAsset(uint64_t guid, std::string assetPathName, HAssetRsrcManager* pAssetRsrcManager);
        ~HTextureAsset();

        virtual void LoadAssetFromDisk();

        HGpuImg* GetGpuImgPtr() { return m_pGpuImg; }

    private:
        uint32_t             m_widthPix;
        uint32_t             m_heightPix;
        uint8_t              m_elePerPix;
        uint8_t              m_bytesPerEle;
        std::vector<float>   m_dataFloat;
        std::vector<uint8_t> m_dataUInt8;

        HGpuImg* m_pGpuImg;
    };

    // GUID - RAM ptr based asset manager.
    // GUID is generated from an asset's path name so it can be used to check whether the rsrc has been loaded into the system.
    // For built in static meshes or data, we need to assign them unique strings.
    class HAssetRsrcManager
    {
    public:
        HAssetRsrcManager();
        ~HAssetRsrcManager();

        // For the editor, the rootDir is the project dir.
        // For the game, the rootDir is the game's dir.
        void UpdateAssetFolderPath(const std::string& rootDir) { m_assetFolderPath = rootDir + "\\assets\\"; }

        std::string GetAssetFolderPath() { return m_assetFolderPath; }

        // We track the reference counter in Loadxxx or ReleaseAsset function.
        // The resource is unloaded when it's reference count become 0.
        // The input asset name path is a relative name path in the asset folder.
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

        std::string m_assetFolderPath;
    };
}