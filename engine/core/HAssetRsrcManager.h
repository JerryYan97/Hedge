#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace Hedge
{
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
    // GUID is generated from an asset's path name.
    class HAssetRsrcManager
    {
    public:
        HAssetRsrcManager();
        ~HAssetRsrcManager();

        uint64_t LoadObjToMeshData(const std::string& objNamePath);
        
    protected:

    private:
        std::unordered_map<uint64_t, void*> m_assetsMap;

    };
}