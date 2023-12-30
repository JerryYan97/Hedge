#include "HAssetRsrcManager.h"
#include "g_builtInModels.h"
#include "Utils.h"
#include "yaml-cpp/yaml.h"
#include "HGpuRsrcManager.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

extern Hedge::HGpuRsrcManager* g_pGpuRsrcManager;

namespace Hedge
{
    // ================================================================================================================
    std::string GetPostFix(
        const std::string& namePath)
    {
        size_t idx = namePath.rfind('.');
        return namePath.substr(idx + 1);
    }

    // ================================================================================================================
    std::string GetNamePathFolderName(
        const std::string& assetNamePath)
    {
        size_t idx = std::min(assetNamePath.rfind('/'), assetNamePath.rfind('\\'));
        if (idx == std::string::npos)
        {
            exit(1);
        }
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
            AssetWrap assetWrap;
            assetWrap.refCounter = 1;

            // Read the type of the asset
            std::string assetConfigFileNamePath;
            std::string assetTypeStr;

            if (assetName.rfind('.') != std::string::npos)
            {
                // virtual texture asset: .vta
                assetConfigFileNamePath = "";
                assetTypeStr = "HTextureAsset";
            }
            else
            {
                assetConfigFileNamePath = m_assetFolderPath + assetName + "\\" + assetName + ".yml";
                YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());
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
                assetWrap.pAsset = new HTextureAsset(guid, m_assetFolderPath + assetName, this);
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
                    idxDataBytesCnt);

                g_pGpuRsrcManager->SendDataToBuffer(m_meshes[i].pIdxDataGpuBuffer, m_meshes[i].idxData.data(), idxDataBytesCnt);

                uint32_t vertDataBytesCnt = m_meshes[i].vertData.size() * sizeof(float);
                m_meshes[i].pVertDataGpuBuffer = g_pGpuRsrcManager->CreateGpuBuffer(
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    vertDataBytesCnt
                );

                g_pGpuRsrcManager->SendDataToBuffer(m_meshes[i].pVertDataGpuBuffer, m_meshes[i].vertData.data(), vertDataBytesCnt);
            }
        }
    }

    // ================================================================================================================
    void HStaticMeshAsset::LoadAssetFromDisk()
    {
        // Read the configuration file
        std::string assetConfigFileNamePath = m_assetPathName + GetNamePathFolderName(m_assetPathName) + ".yml";
        YAML::Node config = YAML::LoadFile(assetConfigFileNamePath.c_str());
         
        // Load other assets according to the configuation file
        // E.g. The material asset on this static mesh asset.
        YAML::Node materials = config["materials"];
        for (uint32_t i = 0; i < materials.size(); i++)
        {
            std::string materialAssetName = materials[i].as<std::string>();
            m_pAssetRsrcManager->LoadAsset(materialAssetName);
        }

        // Load the raw geometry
        std::string rawGeometryFileName = config["src file"].as<std::string>();
        std::string postFix = GetPostFix(rawGeometryFileName);

        if (postFix.compare("gltf") == 0)
        {
            std::string gltfAbsPathName = m_assetPathName + "\\" + rawGeometryFileName;
            LoadGltfRawGeo(gltfAbsPathName);
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
        HAssetRsrcManager* pAssetRsrcManager) :
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_widthPix(0),
        m_heightPix(0),
        m_elePerPix(0),
        m_bytesPerEle(0),
        m_pGpuImg(nullptr)
    {}

    // ================================================================================================================
    HTextureAsset::~HTextureAsset()
    {
        if (m_pGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pGpuImg);
        }
    }

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
        HAsset(guid, assetPathName, pAssetRsrcManager),
        m_baseColorTextureGUID(0),
        m_normalMapGUID(0),
        m_metallicRoughnessGUID(0),
        m_occlusionGUID(0),
        m_pBaseColorGpuImg(nullptr),
        m_pNormalGpuImg(nullptr),
        m_pMetallicRoughnessGpuImg(nullptr),
        m_pOcclusionGpuImg(nullptr)
    {}

    // ================================================================================================================
    HMaterialAsset::~HMaterialAsset()
    {
        if (m_pBaseColorGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pBaseColorGpuImg);
        }
        
        if (m_pNormalGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pNormalGpuImg);
        }

        if (m_pMetallicRoughnessGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pMetallicRoughnessGpuImg);
        }

        if (m_pOcclusionGpuImg != nullptr)
        {
            g_pGpuRsrcManager->DereferGpuImg(m_pOcclusionGpuImg);
        }
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

        HGpuImgCreateInfo gpuImgCreateInfoTemplate{};
        {
            gpuImgCreateInfoTemplate.allocFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            gpuImgCreateInfoTemplate.hasSampler = true;
            gpuImgCreateInfoTemplate.imgSubresRange = imgSubRsrcRange;
            gpuImgCreateInfoTemplate.imgUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            gpuImgCreateInfoTemplate.imgViewType = VK_IMAGE_VIEW_TYPE_2D;
            gpuImgCreateInfoTemplate.samplerInfo = samplerInfo;
        }

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

            m_baseColorTexturePathName = std::to_string(baseColor[0]) + "_" +
                                         std::to_string(baseColor[1]) + "_" +
                                         std::to_string(baseColor[2]) + ".cc"; // Constant Color.

            HGpuImgCreateInfo baseColorGpuImgCreateInfo = gpuImgCreateInfoTemplate;
            baseColorGpuImgCreateInfo.imgExtent = VkExtent3D{ 1, 1, 1 };
            baseColorGpuImgCreateInfo.imgFormat = VK_FORMAT_R8G8B8A8_SRGB;

            m_pBaseColorGpuImg = g_pGpuRsrcManager->CreateGpuImage(baseColorGpuImgCreateInfo);
            g_pGpuRsrcManager->SendDataToImage(m_pBaseColorGpuImg, baseColor, sizeof(baseColor));
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

            m_normalMapPathName = std::to_string(normal[0]) + "_" +
                                  std::to_string(normal[1]) + "_" +
                                  std::to_string(normal[2]) + ".cc"; // Constant Color.

            HGpuImgCreateInfo normalMapGpuImgCreateInfo = gpuImgCreateInfoTemplate;
            normalMapGpuImgCreateInfo.imgExtent = VkExtent3D{ 1, 1, 1 };
            normalMapGpuImgCreateInfo.imgFormat = VK_FORMAT_R8G8B8A8_UNORM;

            m_pNormalGpuImg = g_pGpuRsrcManager->CreateGpuImage(normalMapGpuImgCreateInfo);
            g_pGpuRsrcManager->SendDataToImage(m_pNormalGpuImg, normal, sizeof(normal));
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

            m_metallicRoughnessPathName = std::to_string(metallicRoughness[0]) + "_" +
                                          std::to_string(metallicRoughness[1]) + "_" +
                                          std::to_string(metallicRoughness[2]) + ".cc"; // Constant Color.

            HGpuImgCreateInfo metallicRoughnessMapGpuImgCreateInfo = gpuImgCreateInfoTemplate;
            metallicRoughnessMapGpuImgCreateInfo.imgExtent = VkExtent3D{ 1, 1, 1 };
            metallicRoughnessMapGpuImgCreateInfo.imgFormat = VK_FORMAT_R8G8B8A8_UNORM;

            m_pMetallicRoughnessGpuImg = g_pGpuRsrcManager->CreateGpuImage(metallicRoughnessMapGpuImgCreateInfo);
            g_pGpuRsrcManager->SendDataToImage(m_pMetallicRoughnessGpuImg,
                                               metallicRoughness,
                                               sizeof(metallicRoughness));
        }
        else
        {

        }

        if (isOcclusionFile == false)
        {

        }
        else
        {

        }
    }
}