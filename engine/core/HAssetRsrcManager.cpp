#include "HAssetRsrcManager.h"
#include "g_builtInModels.h"
#include "Utils.h"
#include "yaml-cpp/yaml.h"
#include "HGpuRsrcManager.h"
#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

extern Hedge::HGpuRsrcManager* g_pGpuRsrcManager;

namespace Hedge
{
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
    HTextureType StrToAssetType(std::string str)
    {
        if (str.compare("Cubemap") == 0)
        {
            return HTextureType::CUBEMAP;
        }
        else if (str.compare("Texture2D") == 0)
        {
            return HTextureType::TEXTURE2D;
        }
        else
        {
            assert(1, "Unrecognized texture type.");
            return HTextureType::TEXTURE2D;
        }
    }

    // ================================================================================================================
    uint64_t HAssetRsrcManager::LoadAsset(
        const std::string& assetName)
    {
        uint64_t guid = crc32(assetName.c_str());
        if (m_assetsMap.count(guid) > 0)
        {
            m_assetsMap[guid].refCounter++;
        }
        else
        {
            // TODO: We may need a better design for loading assets, but the current naive design won't cause big
            //       troubles in the future.
            //       Note that each asset has a yaml config file or it's a VTA asset. Thus, some code is unable to be reused.
            AssetWrap assetWrap;
            assetWrap.refCounter = 1;

            // Read the type of the asset
            std::string assetTypeStr;
            YAML::Node config;
            HTextureType texType = HTextureType::TEXTURE2D;

            if (assetName.rfind('.') != std::string::npos)
            {
                // virtual texture asset: .vta
                assetTypeStr = "HTextureAsset";
                texType = HTextureType::VTA;
            }
            else
            {
                std::string assetConfigFileNamePath;
                assetConfigFileNamePath = m_assetFolderPath + assetName + "\\" + assetName + ".yml";
                config = YAML::LoadFile(assetConfigFileNamePath.c_str());
                assetTypeStr = config["asset type"].as<std::string>();
            }

            if (crc32(assetTypeStr.c_str()) == crc32("HStaticMeshAsset"))
            {
                assetWrap.pAsset = new HStaticMeshAsset(guid, m_assetFolderPath + assetName, this);
            }
            else if (crc32(assetTypeStr.c_str()) == crc32("HMaterialAsset"))
            {
                assetWrap.pAsset = new HMaterialAsset(guid, m_assetFolderPath + assetName, this);
            }
            else if (crc32(assetTypeStr.c_str()) == crc32("HTextureAsset"))
            {
                if (texType != HTextureType::VTA)
                {
                    std::string texTypeStr = config["texture asset type"].as<std::string>();
                    texType = StrToAssetType(texTypeStr);
                }
                assetWrap.pAsset = new HTextureAsset(guid, m_assetFolderPath + assetName, this, texType);
            }
            else if (crc32(assetTypeStr.c_str()) == crc32("HIBLAsset"))
            {
                assetWrap.pAsset = new HIBLAsset(guid, m_assetFolderPath + assetName, this);
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
                HAsset* ptr = m_assetsMap.at(guid).pAsset;
                delete ptr;
                m_assetsMap.erase(guid);
            }
        }
    }

    // ================================================================================================================
    void HAssetRsrcManager::ReleaseAllAssets()
    {
        CleanAllAssets();
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
        HAsset(guid, assetPathName, pAssetRsrcManager)
    {
    }

    // ================================================================================================================
    HStaticMeshAsset::~HStaticMeshAsset()
    {
        uint32_t sectionsCnt = GetSectionCounts();
        for (uint32_t i = 0; i < sectionsCnt; i++)
        {
            m_pAssetRsrcManager->ReleaseAsset(m_meshes[i].materialGUID);
            g_pGpuRsrcManager->DereferGpuBuffer(m_meshes[i].pIdxDataGpuBuffer);
            g_pGpuRsrcManager->DereferGpuBuffer(m_meshes[i].pVertDataGpuBuffer);
        }
    }

    // ================================================================================================================
    void HStaticMeshAsset::LoadGltfRawGeo(
        const std::string& namePath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, namePath);
        //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)
        if (!warn.empty()) {
            printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            printf("Err: %s\n", err.c_str());
        }

        if (!ret) {
            printf("Failed to parse glTF\n");
            exit(1);
        }

        // NOTE: TinyGltf loader has already loaded the binary buffer data and the images data.
        const auto& binaryBuffer = model.buffers[0].data;
        const unsigned char* pBufferData = binaryBuffer.data();

        uint32_t meshCnt = model.meshes.size();
        m_meshes.resize(meshCnt);
        for (uint32_t i = 0; i < meshCnt; i++)
        {
            const auto& mesh = model.meshes[i];
            int posIdx = mesh.primitives[0].attributes.at("POSITION");
            int normalIdx = mesh.primitives[0].attributes.at("NORMAL");
            int tangentIdx = mesh.primitives[0].attributes.at("TANGENT");
            int uvIdx = mesh.primitives[0].attributes.at("TEXCOORD_0");
            int indicesIdx = mesh.primitives[0].indices;
            int materialIdx = mesh.primitives[0].material;

            // Elements notes:
            // Position: float3, normal: float3, tangent: float4, texcoord: float2.

            // Setup the vertex buffer and the index buffer
            const auto& posAccessor = model.accessors[posIdx];
            int posAccessorByteOffset = posAccessor.byteOffset;
            int posAccessorEleCnt = posAccessor.count; // Assume a position element is a float3.

            const auto& normalAccessor = model.accessors[normalIdx];
            int normalAccessorByteOffset = normalAccessor.byteOffset;
            int normalAccessorEleCnt = normalAccessor.count;

            const auto& tangentAccessor = model.accessors[tangentIdx];
            int tangentAccessorByteOffset = tangentAccessor.byteOffset;
            int tangentAccessorEleCnt = tangentAccessor.count;

            const auto& uvAccessor = model.accessors[uvIdx];
            int uvAccessorByteOffset = uvAccessor.byteOffset;
            int uvAccessorEleCnt = uvAccessor.count;

            const auto& idxAccessor = model.accessors[indicesIdx];
            int idxAccessorByteOffset = idxAccessor.byteOffset;
            int idxAccessorEleCnt = idxAccessor.count;

            // NOTE: Buffer views are just division of the buffer for the flight helmet model.
            // SCALAR is in one buffer view. FLOAT2 in one. FLOAT3 in one. and FLOAT3 in one...
            // Maybe they can be more
            // A buffer view represents a contiguous segment of data in a buffer, defined by a byte offset into the buffer specified 
            // in the byteOffset property and a total byte length specified by the byteLength property of the buffer view.
            const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
            const auto& normalBufferView = model.bufferViews[normalAccessor.bufferView];
            const auto& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
            const auto& uvBufferView = model.bufferViews[uvAccessor.bufferView];
            const auto& idxBufferView = model.bufferViews[idxAccessor.bufferView];

            // We assume that the idx, position, normal, uv and tangent are not interleaved.
            // TODO: Even though they are interleaved, we can use a function to read out the data by making use of the stride bytes count.

            // Assmue the data and element type of the index is uint16_t.
            int idxBufferOffset = idxAccessorByteOffset + idxBufferView.byteOffset;
            int idxBufferByteCnt = sizeof(uint16_t) * idxAccessor.count;
            m_meshes[i].idxData.resize(idxAccessor.count);
            memcpy(m_meshes[i].idxData.data(), &pBufferData[idxBufferOffset], idxBufferByteCnt);

            // Assmue the data and element type of the position is float3
            int posBufferOffset = posAccessorByteOffset + posBufferView.byteOffset;
            int posBufferByteCnt = sizeof(float) * 3 * posAccessor.count;
            float* pPosData = new float[3 * posAccessor.count];
            memcpy(pPosData, &pBufferData[posBufferOffset], posBufferByteCnt);

            // Assmue the data and element type of the normal is float3.
            int normalBufferOffset = normalAccessorByteOffset + normalBufferView.byteOffset;
            int normalBufferByteCnt = sizeof(float) * 3 * normalAccessor.count;
            float* pNomralData = new float[3 * normalAccessor.count];
            memcpy(pNomralData, &pBufferData[normalBufferOffset], normalBufferByteCnt);

            // Assmue the data and element type of the tangent is float4.
            int tangentBufferOffset = tangentAccessorByteOffset + tangentBufferView.byteOffset;
            int tangentBufferByteCnt = sizeof(float) * 4 * tangentAccessor.count;
            float* pTangentData = new float[4 * tangentAccessor.count];
            memcpy(pTangentData, &pBufferData[tangentBufferOffset], tangentBufferByteCnt);

            // Assume the data and element type of the texcoord is float2.
            int uvBufferOffset = uvAccessorByteOffset + uvBufferView.byteOffset;
            int uvBufferByteCnt = sizeof(float) * 2 * uvAccessor.count;
            float* pUvData = new float[2 * uvAccessor.count];
            memcpy(pUvData, &pBufferData[uvBufferOffset], uvBufferByteCnt);

            // Assemble the vert buffer, fill the mesh information and send vert buffer and idx buffer to VkBuffer.

            // Fill the vert buffer
            int vertBufferByteCnt = posBufferByteCnt + normalBufferByteCnt + tangentBufferByteCnt + uvBufferByteCnt;
            int vertBufferDwordCnt = vertBufferByteCnt / sizeof(float);

            m_meshes[i].vertData.resize(vertBufferDwordCnt);

            // The count of [pos, normal, tangent, uv] is equal to posAccessor/normalAccessor/tangentAccessor/uvAccessor.count.
            // [3 floats, 3 floats, 4 floats, 2 floats] --> 12 floats.
            for (uint32_t vertIdx = 0; vertIdx < posAccessor.count; vertIdx++)
            {
                // pos -- 3 floats
                m_meshes[i].vertData[12 * vertIdx] = pPosData[3 * vertIdx];
                m_meshes[i].vertData[12 * vertIdx + 1] = pPosData[3 * vertIdx + 1];
                m_meshes[i].vertData[12 * vertIdx + 2] = pPosData[3 * vertIdx + 2];

                // normal -- 3 floats
                m_meshes[i].vertData[12 * vertIdx + 3] = pNomralData[3 * vertIdx];
                m_meshes[i].vertData[12 * vertIdx + 4] = pNomralData[3 * vertIdx + 1];
                m_meshes[i].vertData[12 * vertIdx + 5] = pNomralData[3 * vertIdx + 2];

                // tangent -- 4 floats
                m_meshes[i].vertData[12 * vertIdx + 6] = pTangentData[4 * vertIdx];
                m_meshes[i].vertData[12 * vertIdx + 7] = pTangentData[4 * vertIdx + 1];
                m_meshes[i].vertData[12 * vertIdx + 8] = pTangentData[4 * vertIdx + 2];
                m_meshes[i].vertData[12 * vertIdx + 9] = pTangentData[4 * vertIdx + 3];

                // uv -- 2 floats
                m_meshes[i].vertData[12 * vertIdx + 10] = pUvData[2 * vertIdx];
                m_meshes[i].vertData[12 * vertIdx + 11] = pUvData[2 * vertIdx + 1];
            }

            // Create the VkBuffer for the idx and vert buffer -- NOTE: For optimization, we may want to use the mesh
            // files' to manage GPU rsrc so that different static meshs that share the same raw geometry mesh can also
            // share the same GPU idx and vert buffer.
            {
                uint32_t idxDataBytesCnt = m_meshes[i].idxData.size() * sizeof(uint16_t);
                m_meshes[i].pIdxDataGpuBuffer = g_pGpuRsrcManager->CreateGpuBuffer(
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    idxDataBytesCnt, "IdxBuffer");

                g_pGpuRsrcManager->SendDataToBuffer(m_meshes[i].pIdxDataGpuBuffer, m_meshes[i].idxData.data(), idxDataBytesCnt);

                uint32_t vertDataBytesCnt = m_meshes[i].vertData.size() * sizeof(float);
                m_meshes[i].pVertDataGpuBuffer = g_pGpuRsrcManager->CreateGpuBuffer(
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    vertDataBytesCnt, "VertBuffer"
                );

                g_pGpuRsrcManager->SendDataToBuffer(m_meshes[i].pVertDataGpuBuffer, m_meshes[i].vertData.data(), vertDataBytesCnt);
            }
        }
    }

    // ================================================================================================================
    void HStaticMeshAsset::LoadObjRawGeo(const std::string& namePath)
    {
        tinyobj::ObjReaderConfig readerConfig;
        tinyobj::ObjReader objReader;

        objReader.ParseFromFile(namePath, readerConfig);

        auto& shapes = objReader.GetShapes();
        auto& attrib = objReader.GetAttrib();

        // We assume that this test only has one shape
        assert(shapes.size() == 1, "This application only accepts one shape!");



    }

    // ================================================================================================================
    void HStaticMeshAsset::LoadAssetFromDisk()
    {
        // Read the configuration file
        std::string assetConfigFileNamePath = m_assetPathName + "\\" + GetNamePathFolderName(m_assetPathName) + ".yml";
        YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());

        // Load other assets according to the configuation file
        // E.g. The material asset on this static mesh asset.
        YAML::Node materials = config["materials"];
        m_meshes.resize(materials.size());

        for (uint32_t i = 0; i < materials.size(); i++)
        {
            std::string materialAssetName = materials[i].as<std::string>();
            m_meshes[i].materialGUID = m_pAssetRsrcManager->LoadAsset(materialAssetName);
            m_meshes[i].materialPathName = materialAssetName;
        }

        // Load the raw geometry
        std::string rawGeometryFileName = config["src file"].as<std::string>();
        std::string postFix = GetPostFix(rawGeometryFileName);

        if (postFix.compare("gltf") == 0)
        {
            std::string gltfAbsPathName = m_assetPathName + "\\" + rawGeometryFileName;
            LoadGltfRawGeo(gltfAbsPathName);
        }
        else if (postFix.compare("obj") == 0)
        {
            std::string objAbsPathName = m_assetPathName + "\\" + rawGeometryFileName;
            LoadObjRawGeo(objAbsPathName);
        }
        else
        {
            exit(1);
        }
    }

    // ================================================================================================================
    HGpuBuffer* HStaticMeshAsset::GetIdxGpuBuffer(
        uint32_t i)
    {
        return m_meshes[i].pIdxDataGpuBuffer;
    }

    // ================================================================================================================
    HGpuBuffer* HStaticMeshAsset::GetVertGpuBuffer(
        uint32_t i)
    {
        return m_meshes[i].pVertDataGpuBuffer;
    }
    
    // ================================================================================================================
    HTextureAsset::HTextureAsset(
        uint64_t    guid,
        std::string assetPathName,
        HAssetRsrcManager* pAssetRsrcManager,
        HTextureType texType) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_widthPix(0),
        m_heightPix(0),
        m_elePerPix(0),
        m_bytesPerEle(0),
        m_pGpuImg(nullptr),
        m_texAssetType(texType)
    {
        /*
        switch (m_texAssetType)
        {
        case HTextureType::VTA:
            SetTextureInfoAsVta();
            break;
        case HTextureType::CUBEMAP:
            SetTextureInfoAsCubemap();
            break;
        case HTextureType::TEXTURE2D:
            SetTextureInfoAs2D();
            break;
        default:
            assert(1, "Unrecognized texture type.");
            break;
        }
        */
    }

    // ================================================================================================================
    HTextureAsset::~HTextureAsset()
    {
        if (m_pGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pGpuImg);
        }
    }

    // ================================================================================================================
    static VkImageSubresourceRange GenImgSubrsrcRange(uint32_t layerCnt = 1, uint32_t mipLevels = 1)
    {
        VkImageSubresourceRange imgSubRsrcRange{};
        {
            imgSubRsrcRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imgSubRsrcRange.baseArrayLayer = 0;
            imgSubRsrcRange.layerCount = layerCnt;
            imgSubRsrcRange.baseMipLevel = 0;
            imgSubRsrcRange.levelCount = mipLevels;
        }

        return imgSubRsrcRange;
    }

    // ================================================================================================================
    static VkSamplerCreateInfo GenSamplerCreateInfo()
    {
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

        return samplerInfo;
    }

    // ================================================================================================================
    static VkBufferImageCopy GenBufferImgCopyInfo(uint32_t width, uint32_t height, uint32_t layerCnt = 1, uint32_t mipLevelId = 0)
    {
        VkBufferImageCopy bufToImgCopyInfo{};
        {
            VkExtent3D extent{};
            {
                extent.width = width;
                extent.height = height;
                extent.depth = 1;
            }

            bufToImgCopyInfo.bufferRowLength = width;
            bufToImgCopyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufToImgCopyInfo.imageSubresource.mipLevel = mipLevelId;
            bufToImgCopyInfo.imageSubresource.baseArrayLayer = 0;
            bufToImgCopyInfo.imageSubresource.layerCount = layerCnt;
            bufToImgCopyInfo.imageExtent = extent;
        }

        return bufToImgCopyInfo;
    }

    // ================================================================================================================
    static void GenVtaTextureCreateInitInfo(HGpuImgCreateInfo& oImgCreateInfo, VkBufferImageCopy& oBufferImgCopyInfo)
    {
        VkImageSubresourceRange imgSubRsrcRange = GenImgSubrsrcRange();

        VkSamplerCreateInfo samplerInfo = GenSamplerCreateInfo();

        HGpuImgCreateInfo gpuImgCreateInfoTemplate{};
        {
            gpuImgCreateInfoTemplate.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            gpuImgCreateInfoTemplate.hasSampler = true;
            gpuImgCreateInfoTemplate.imgSubresRange = imgSubRsrcRange;
            gpuImgCreateInfoTemplate.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            gpuImgCreateInfoTemplate.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
            gpuImgCreateInfoTemplate.samplerInfo = samplerInfo;
            gpuImgCreateInfoTemplate.imgExtent = VkExtent3D{ 1, 1, 1 };
            gpuImgCreateInfoTemplate.imgFormat = VK_FORMAT_R8G8B8A8_UNORM;
            gpuImgCreateInfoTemplate.imgTiling = VK_IMAGE_TILING_OPTIMAL;
        }
        oImgCreateInfo = gpuImgCreateInfoTemplate;

        oBufferImgCopyInfo = GenBufferImgCopyInfo(1, 1);
    }

    // ================================================================================================================
    static void GenCubemapTextureCreateInitInfo(VkExtent2D widthHeightOneLayer, HGpuImgCreateInfo& oImgCreateInfo, VkBufferImageCopy& oBufferImgCopyInfo, int mipLevels = 1)
    {
        // Assume HDR and 6 faces.
        VkImageSubresourceRange imgSubRsrcRange = GenImgSubrsrcRange(6, mipLevels);
        VkSamplerCreateInfo samplerInfo = GenSamplerCreateInfo();
        HGpuImgCreateInfo gpuImgCreateInfoTemplate{};
        {
            gpuImgCreateInfoTemplate.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            gpuImgCreateInfoTemplate.hasSampler = true;
            gpuImgCreateInfoTemplate.imgSubresRange = imgSubRsrcRange;
            gpuImgCreateInfoTemplate.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            gpuImgCreateInfoTemplate.imgViewType = VK_IMAGE_VIEW_TYPE_CUBE;
            gpuImgCreateInfoTemplate.samplerInfo = samplerInfo;
            gpuImgCreateInfoTemplate.imgExtent = VkExtent3D{ widthHeightOneLayer.width, widthHeightOneLayer.height / 6, 1 };
            gpuImgCreateInfoTemplate.imgFormat = VK_FORMAT_R32G32B32_SFLOAT;
            gpuImgCreateInfoTemplate.imgTiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_LINEAR; Nvidia needs it to be optimal.
            gpuImgCreateInfoTemplate.imgCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
        oImgCreateInfo = gpuImgCreateInfoTemplate;

        oBufferImgCopyInfo = GenBufferImgCopyInfo(widthHeightOneLayer.width, widthHeightOneLayer.height / 6, 6);
    }

    // ================================================================================================================
    static void Gen2DTextureCreateInitInfo(VkExtent2D widthHeightOneLayer, HGpuImgCreateInfo& oImgCreateInfo, VkBufferImageCopy& oBufferImgCopyInfo)
    {
        VkImageSubresourceRange imgSubRsrcRange = GenImgSubrsrcRange();
        VkSamplerCreateInfo samplerInfo = GenSamplerCreateInfo();
        HGpuImgCreateInfo gpuImgCreateInfoTemplate{};
        {
            gpuImgCreateInfoTemplate.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            gpuImgCreateInfoTemplate.hasSampler = true;
            gpuImgCreateInfoTemplate.imgSubresRange = imgSubRsrcRange;
            gpuImgCreateInfoTemplate.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            gpuImgCreateInfoTemplate.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
            gpuImgCreateInfoTemplate.samplerInfo = samplerInfo;
            gpuImgCreateInfoTemplate.imgExtent = VkExtent3D{ widthHeightOneLayer.width, widthHeightOneLayer.height, 1 };
            gpuImgCreateInfoTemplate.imgFormat = VK_FORMAT_R32G32B32_SFLOAT;
            gpuImgCreateInfoTemplate.imgTiling = VK_IMAGE_TILING_LINEAR;
            // gpuImgCreateInfoTemplate.imgCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
        oImgCreateInfo = gpuImgCreateInfoTemplate;

        oBufferImgCopyInfo = GenBufferImgCopyInfo(widthHeightOneLayer.width, widthHeightOneLayer.height);
    }

    // ================================================================================================================
    void HTextureAsset::LoadAssetFromDisk()
    {
        // std::string postFix = GetPostFix(m_assetPathName);
        // Always assume 4 color channels in a vta.
        if (m_texAssetType == HTextureType::VTA)
        {
            // Virtual texture asset. Pure consant color image.
            m_widthPix = 1;
            m_heightPix = 1;
            m_elePerPix = 4;
            m_bytesPerEle = sizeof(float); // Bytes per channel.
            
            // Get the pure color value from the name
            float data[4] = {};
            std::string nameWithPostFix = GetNamePathFolderName(m_assetPathName);
            uint32_t dotIdx = nameWithPostFix.rfind('.');
            std::cout << nameWithPostFix.substr(0, dotIdx) << std::endl;
            GetColorValFromName(nameWithPostFix.substr(0, dotIdx), data);
            
            m_dataFloat = std::vector<float>(data, data + 4);
            HGpuImgCreateInfo imgCreateInfo{};
            VkBufferImageCopy bufferImgCopy{};

            GenVtaTextureCreateInitInfo(imgCreateInfo, bufferImgCopy);
            m_pGpuImg = g_pGpuRsrcManager->CreateGpuImage(imgCreateInfo, m_assetPathName);

            m_dataUInt8.push_back(255 * m_dataFloat[0]);
            m_dataUInt8.push_back(255 * m_dataFloat[1]);
            m_dataUInt8.push_back(255 * m_dataFloat[2]);
            m_dataUInt8.push_back(255 * m_dataFloat[3]);

            g_pGpuRsrcManager->SendDataToImage(m_pGpuImg, bufferImgCopy, m_dataUInt8.data(), sizeof(uint8_t) * m_dataUInt8.size());
            g_pGpuRsrcManager->TransImageLayout(m_pGpuImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else if (m_texAssetType == HTextureType::CUBEMAP)
        {
            // Read the configuration file
            std::string assetConfigFileNamePath = m_assetPathName + "\\" + GetNamePathFolderName(m_assetPathName) + ".yml";
            YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());
            std::string fileName = config["src file"].as<std::string>();
            std::string cubemapTexFilePathName = m_assetPathName + "\\" + fileName;

            int width, height, nrComponents;
            float* pData = stbi_loadf(cubemapTexFilePathName.c_str(), &width, &height, &nrComponents, 0);
            m_dataFloat.resize(width * height * nrComponents);
            memcpy(m_dataFloat.data(), pData, sizeof(float) * width * height * nrComponents);
            delete pData;

            HGpuImgCreateInfo imgCreateInfo{};
            VkBufferImageCopy bufferImgCopy{};

            // Cubemap texture asset.
            GenCubemapTextureCreateInitInfo(VkExtent2D{(uint32_t)width, (uint32_t)height}, imgCreateInfo, bufferImgCopy);
            m_pGpuImg = g_pGpuRsrcManager->CreateGpuImage(imgCreateInfo, m_assetPathName);
            g_pGpuRsrcManager->SendDataToImage(m_pGpuImg, bufferImgCopy, m_dataFloat.data(), sizeof(float) * m_dataFloat.size());
            g_pGpuRsrcManager->TransImageLayout(m_pGpuImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else if (m_texAssetType == HTextureType::TEXTURE2D)
        {
            // 2D texture asset.

        }
        else if (m_texAssetType == HTextureType::Mipmap)
        {

        }
        else
        {
            assert(1, "Unrecognized texture type.");
        }
    }

    // ================================================================================================================
    void HTextureAsset::GetColorValFromName(
        const std::string& name,
        float*             pVal)
    {
        std::vector<int> underlineIdxs = FindLocations(name, '_');
        int dotIdx = name.rfind('.');
        
        int prevUnderlineIdx = -1;
        for (uint32_t i = 0; i < underlineIdxs.size(); i++)
        {
            int curUnderlineIdx = underlineIdxs[i];
            std::string curValStr = name.substr(prevUnderlineIdx + 1, curUnderlineIdx - prevUnderlineIdx - 1);
            prevUnderlineIdx = curUnderlineIdx;
            pVal[i] = std::stof(curValStr);
        }

        // The last value
        std::string lastValStr = name.substr(prevUnderlineIdx + 1, dotIdx - prevUnderlineIdx - 1);
        pVal[3] = std::stof(lastValStr);
    }
    
    // ================================================================================================================
    // Material doesn't have a src file.
    HMaterialAsset::HMaterialAsset(
        uint64_t    guid,
        std::string assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_baseColorTextureGUID(0),
        m_normalMapGUID(0),
        m_metallicRoughnessGUID(0),
        m_occlusionGUID(0)
    {}

    // ================================================================================================================
    HMaterialAsset::~HMaterialAsset()
    {
        m_pAssetRsrcManager->ReleaseAsset(m_baseColorTextureGUID);
        m_pAssetRsrcManager->ReleaseAsset(m_normalMapGUID);
        m_pAssetRsrcManager->ReleaseAsset(m_metallicRoughnessGUID);
        m_pAssetRsrcManager->ReleaseAsset(m_occlusionGUID);
    }

    // ================================================================================================================
    void HMaterialAsset::LoadAssetFromDisk()
    {
        std::cout << "Hello Material" << std::endl;

        std::string folderName = GetNamePathFolderName(m_assetPathName);
        std::string assetPathName = m_assetPathName + "/" + folderName + ".yml";

        YAML::Node config = YAML::LoadFile(assetPathName.c_str());;
        bool isBaseColorFile = config["base color"].IsScalar();
        bool isNormalFile = config["normal map"].IsScalar();
        bool isMetallicRoughnessFile = config["metalic roughness"].IsScalar();
        bool isOcclusionFile = config["occlusion"].IsScalar();

        if (isBaseColorFile == false)
        {
            // Make a four component padding since we will always use the RGBA format for Nvidia compatibility.
            YAML::Node baseColorSeq = config["base color"];
            float baseColor[4] = {
                baseColorSeq[0].as<float>(),
                baseColorSeq[1].as<float>(),
                baseColorSeq[2].as<float>(),
                1.0
            };

            // virtual texture asset: .vta
            m_baseColorTexturePathName = std::to_string(baseColor[0]) + "_" +
                                         std::to_string(baseColor[1]) + "_" +
                                         std::to_string(baseColor[2]) + "_" +
                                         std::to_string(baseColor[3]) + ".vta";

            m_baseColorTextureGUID = m_pAssetRsrcManager->LoadAsset(m_baseColorTexturePathName);
        }
        else
        {

        }

        if (isNormalFile == false)
        {
            YAML::Node normalMapSeq = config["normal map"];
            float normal[4] = {
                normalMapSeq[0].as<float>(),
                normalMapSeq[1].as<float>(),
                normalMapSeq[2].as<float>(),
                1.0
            };

            // virtual texture asset: .vta
            m_normalMapPathName = std::to_string(normal[0]) + "_" +
                                  std::to_string(normal[1]) + "_" +
                                  std::to_string(normal[2]) + "_" + 
                                  std::to_string(normal[3]) + ".vta";

            m_normalMapGUID = m_pAssetRsrcManager->LoadAsset(m_normalMapPathName);
        }
        else
        {

        }

        if (isMetallicRoughnessFile == false)
        {
            YAML::Node metallicRoughnessSeq = config["metalic roughness"];
            float metallicRoughness[4] = {
                metallicRoughnessSeq[0].as<float>(),
                metallicRoughnessSeq[1].as<float>(),
                1.0,
                1.0
            };

            // virtual texture asset: .vta
            m_metallicRoughnessPathName = std::to_string(metallicRoughness[0]) + "_" +
                                          std::to_string(metallicRoughness[1]) + "_" +
                                          std::to_string(metallicRoughness[2]) + "_" +
                                          std::to_string(metallicRoughness[3]) + ".vta";

            m_metallicRoughnessGUID = m_pAssetRsrcManager->LoadAsset(m_metallicRoughnessPathName);
        }
        else
        {

        }

        if (isOcclusionFile == false)
        {
            YAML::Node occlusionNode = config["occlusion"];
            float occlusion[4] = {
                occlusionNode[0].as<float>(),
                1.f, 1.f, 1.f
            };

            m_occlusionPathName = std::to_string(occlusion[0]) + "_" +
                                  std::to_string(occlusion[1]) + "_" +
                                  std::to_string(occlusion[2]) + "_" +
                                  std::to_string(occlusion[3]) + ".vta";

            m_occlusionGUID = m_pAssetRsrcManager->LoadAsset(m_occlusionPathName);
        }
        else
        {

        }
    }

    // ================================================================================================================
    HIBLAsset::HIBLAsset(
        uint64_t guid,
        std::string assetPathName,
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_diffuseCubemapGpuImg(nullptr),
        m_prefilterEnvCubemapGpuImg(nullptr),
        m_envBrdfGpuImg(nullptr),
        m_iblMaxMipLevels(0)
    {
    }

    // ================================================================================================================
    HIBLAsset::~HIBLAsset()
    {
        if (m_diffuseCubemapGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_diffuseCubemapGpuImg);
        }

        if (m_prefilterEnvCubemapGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_prefilterEnvCubemapGpuImg);
        }

        if (m_envBrdfGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_envBrdfGpuImg);
        }
    }

    // ================================================================================================================
    void HIBLAsset::LoadAssetFromDisk()
    {
        std::string folderName = GetNamePathFolderName(m_assetPathName);
        m_envBrdfPathName = m_assetPathName + "/envBrdf.hdr";
        m_diffuseCubemapPathName = m_assetPathName + "/diffuse_irradiance_cubemap.hdr";
        m_prefilterEnvCubemapPathName = m_assetPathName + "/prefilterEnvMaps";

        /*
        const std::filesystem::path prefilterEnvMapsFolderPath(m_prefilterEnvCubemapPathName);
        m_iblMaxMipLevels = 0;
        for (auto& p : std::filesystem::directory_iterator(prefilterEnvMapsFolderPath))
        {
            m_iblMaxMipLevels++;
        }
        */

        // Load the diffuse cubemap
        {
            std::vector<float> dataFloat;
            int width, height, nrComponents;
            float* pData = stbi_loadf(m_diffuseCubemapPathName.c_str(), &width, &height, &nrComponents, 0);
            dataFloat.resize(width * height * nrComponents);
            memcpy(dataFloat.data(), pData, sizeof(float) * width * height * nrComponents);
            delete pData;

            HGpuImgCreateInfo imgCreateInfo{};
            VkBufferImageCopy bufferImgCopy{};

            // Cubemap texture asset.
            GenCubemapTextureCreateInitInfo(VkExtent2D{(uint32_t)width, (uint32_t)height}, imgCreateInfo, bufferImgCopy);
            m_diffuseCubemapGpuImg = g_pGpuRsrcManager->CreateGpuImage(imgCreateInfo, m_assetPathName);
            g_pGpuRsrcManager->SendDataToImage(m_diffuseCubemapGpuImg, bufferImgCopy, dataFloat.data(), sizeof(float) * dataFloat.size());
            g_pGpuRsrcManager->TransImageLayout(m_diffuseCubemapGpuImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        // Load the environment BRDF
        {
            std::vector<float> dataFloat;
            int width, height, nrComponents;
            float* pData = stbi_loadf(m_envBrdfPathName.c_str(), &width, &height, &nrComponents, 0);
            dataFloat.resize(width * height * nrComponents);
            memcpy(dataFloat.data(), pData, sizeof(float) * width * height * nrComponents);
            delete pData;

            HGpuImgCreateInfo imgCreateInfo{};
            VkBufferImageCopy bufferImgCopy{};

            // 2D texture
            Gen2DTextureCreateInitInfo(VkExtent2D{(uint32_t)width, (uint32_t)height}, imgCreateInfo, bufferImgCopy);
            m_envBrdfGpuImg = g_pGpuRsrcManager->CreateGpuImage(imgCreateInfo, m_assetPathName);
            g_pGpuRsrcManager->SendDataToImage(m_envBrdfGpuImg, bufferImgCopy, dataFloat.data(), sizeof(float) * dataFloat.size());
            g_pGpuRsrcManager->TransImageLayout(m_envBrdfGpuImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        // Load the prefilter environment cubemaps
        {
            std::vector<std::string> mipImgNames;
            GetAllFileNames(m_prefilterEnvCubemapPathName, mipImgNames);
            m_iblMaxMipLevels = mipImgNames.size();

            for (uint32_t i = 0; i < mipImgNames.size(); i++)
            {
                std::string mipImgPathName = m_prefilterEnvCubemapPathName + "/" + mipImgNames[i];
                std::vector<float> dataFloat;
                int width, height, nrComponents;
                float* pData = stbi_loadf(mipImgPathName.c_str(), &width, &height, &nrComponents, 0);
                dataFloat.resize(width * height * nrComponents);
                memcpy(dataFloat.data(), pData, sizeof(float) * width * height * nrComponents);
                delete pData;

                HGpuImgCreateInfo imgCreateInfo{};
                VkBufferImageCopy bufferImgCopy{};

                // Cubemap texture asset.
                if (i == 0)
                {
                    GenCubemapTextureCreateInitInfo(VkExtent2D{(uint32_t)width, (uint32_t)height}, imgCreateInfo, bufferImgCopy, m_iblMaxMipLevels);
                    m_prefilterEnvCubemapGpuImg = g_pGpuRsrcManager->CreateGpuImage(imgCreateInfo, m_assetPathName);
                }
                
                bufferImgCopy = GenBufferImgCopyInfo(width, width, 6, i);
                g_pGpuRsrcManager->SendDataToImage(m_prefilterEnvCubemapGpuImg, bufferImgCopy, dataFloat.data(), sizeof(float) * dataFloat.size());
            }

            g_pGpuRsrcManager->TransImageLayout(m_prefilterEnvCubemapGpuImg, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
}