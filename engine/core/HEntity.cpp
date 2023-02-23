#include "HEntity.h"
#include "../scene/HScene.h"
#include "../render/HRenderManager.h"
#include "HComponent.h"
#include "Utils.h"
#include <sstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Hedge
{
    // ================================================================================================================
    HEntity::HEntity()
        : m_pScene(nullptr),
          m_entityHandle(0)
    {}

    // ================================================================================================================
    HEntity::~HEntity()
    {}

    // ================================================================================================================
    void HEntity::CreateInSceneInternal(
        HScene*  pScene,
        uint32_t handle)
    {
        m_pScene         = pScene;
        m_entityHandle   = handle;
    }

    // ================================================================================================================
    template<typename Type, typename... Args>
    void HEntity::AddComponent(
        Args &&...args)
    {
        m_pScene->EntityAddComponent<Type>(m_entityHandle, std::forward<Args>(args)...);
    }

    // ================================================================================================================
    template<typename T>
    T& HEntity::GetComponent()
    {
        return m_pScene->EntityGetComponent<T>(m_entityHandle);
    }

    // ================================================================================================================
    HCubeEntity::~HCubeEntity()
    {
        StaticMeshComponent& meshComponent = GetComponent<StaticMeshComponent>();
        delete meshComponent.m_pIdx;
        delete meshComponent.m_pVert;
    }

    // ================================================================================================================
    uint32_t TinyObjToVertBufAndIdxBuf(
        const std::vector<tinyobj::shape_t>& inshapes, 
        const tinyobj::attrib_t& inattrib, 
        float** ppVertBuffer,
        uint32_t** ppIdxBuffer)
    {
        // TODO: We should be able to rule out redundant vertices that use same (pos + norm + uv);
        uint32_t vertNum = inshapes[0].mesh.indices.size();
        float* vertBuf = new float[vertNum * VertFloatNum];
        uint32_t* idxBuf = new uint32_t[vertNum];

        float vertBufCheck[288];
        uint32_t idxBufCheck[36];

        for (uint32_t i = 0; i < vertNum; i++)
        {
            uint32_t tinyObjPosId = inshapes[0].mesh.indices[i].vertex_index;
            uint32_t tinyObjUvId = inshapes[0].mesh.indices[i].texcoord_index;
            uint32_t tinyObjNormalId = inshapes[0].mesh.indices[i].normal_index;

            // Pos
            vertBuf[VertFloatNum * i] = inattrib.vertices[tinyObjPosId * 3];
            vertBuf[VertFloatNum * i + 1] = inattrib.vertices[tinyObjPosId * 3 + 1];
            vertBuf[VertFloatNum * i + 2] = inattrib.vertices[tinyObjPosId * 3 + 2];

            // Normal
            vertBuf[VertFloatNum * i + 3] = inattrib.normals[tinyObjNormalId * 3];
            vertBuf[VertFloatNum * i + 4] = inattrib.normals[tinyObjNormalId * 3 + 1];
            vertBuf[VertFloatNum * i + 5] = inattrib.normals[tinyObjNormalId * 3 + 2];

            // Uv
            vertBuf[VertFloatNum * i + 6] = inattrib.texcoords[tinyObjUvId * 2];
            vertBuf[VertFloatNum * i + 7] = inattrib.texcoords[tinyObjUvId * 2 + 1];

            idxBuf[i] = i;
        }

        *ppVertBuffer = vertBuf;
        *ppIdxBuffer = idxBuf;

        memcpy(vertBufCheck, vertBuf, sizeof(vertBufCheck));
        memcpy(idxBufCheck, idxBuf, sizeof(idxBufCheck));

        return vertNum;
    }

    // ================================================================================================================
    void HCubeEntity::OnDefineEntity()
    {
        //
        float pos[3] = {0.f, 0.f, 2.f};
        float rot[3] = {0.f, 0.f, 0.f};
        float scale[3] = {1.f, 1.f, 1.f};

        std::string cubeObj("o Cube \n\
v 1.000000 1.000000 -1.000000 \n\
v 1.000000 -1.000000 -1.000000 \n\
v 1.000000 1.000000 1.000000 \n\
v 1.000000 -1.000000 1.000000 \n\
v -1.000000 1.000000 -1.000000 \n\
v -1.000000 -1.000000 -1.000000 \n\
v -1.000000 1.000000 1.000000 \n\
v -1.000000 -1.000000 1.000000 \n\
vt 0.875000 0.500000 \n\
vt 0.625000 0.750000 \n\
vt 0.625000 0.500000 \n\
vt 0.375000 1.000000 \n\
vt 0.375000 0.750000 \n\
vt 0.625000 0.000000 \n\
vt 0.375000 0.250000 \n\
vt 0.375000 0.000000 \n\
vt 0.375000 0.500000 \n\
vt 0.125000 0.750000 \n\
vt 0.125000 0.500000 \n\
vt 0.625000 0.250000 \n\
vt 0.875000 0.750000 \n\
vt 0.625000 1.000000 \n\
vn 0.0000 1.0000 0.0000 \n\
vn 0.0000 0.0000 1.0000 \n\
vn -1.0000 0.0000 0.0000 \n\
vn 0.0000 -1.0000 0.0000 \n\
vn 1.0000 0.0000 0.0000 \n\
vn 0.0000 0.0000 -1.0000 \n\
s off \n\
f 5/1/1 3/2/1 1/3/1 \n\
f 3/2/2 8/4/2 4/5/2 \n\
f 7/6/3 6/7/3 8/8/3 \n\
f 2/9/4 8/10/4 6/11/4 \n\
f 1/3/5 4/5/5 2/9/5 \n\
f 5/12/6 2/9/6 6/7/6 \n\
f 5/1/1 7/13/1 3/2/1 \n\
f 3/2/2 7/14/2 8/4/2 \n\
f 7/6/3 5/12/3 6/7/3 \n\
f 2/9/4 4/5/4 8/10/4 \n\
f 1/3/5 3/2/5 4/5/5 \n\
f 5/12/6 1/3/6 2/9/6");

        std::vector<tinyobj::material_t> materials;
        std::map<std::string, uint32_t> textures;
        tinyobj::attrib_t inattrib;
        std::vector<tinyobj::shape_t> inshapes;
        std::string warn;
        std::string err;
        std::istringstream istr(cubeObj);

        tinyobj::LoadObj(&inattrib, &inshapes, &materials, &warn, &err, &istr);

        float* pVertBuffer;
        uint32_t* pIdxBuffer;
        uint32_t vertNum = TinyObjToVertBufAndIdxBuf(inshapes, inattrib, &pVertBuffer, &pIdxBuffer);

        AddComponent<StaticMeshComponent>(pIdxBuffer,
                                          pVertBuffer,
                                          vertNum,
                                          VertFloatNum * sizeof(float) * vertNum);
    }

    // ================================================================================================================
    HCameraEntity::~HCameraEntity()
    {}

    // ================================================================================================================
    void HCameraEntity::OnDefineEntity()
    {
        float pos[3] = { 0.f, 2.f, -1.f };
        float rot[4] = { 0.f, 0.f, 0.f, 0.f };
        float scale[3] = { 1.f, 1.f, 1.f };

        AddComponent<TransformComponent>(pos, rot, scale);

        float view[3] = { 0.f, -2.f, 1.f };
        float up[3] = { 0.f, 1.f, 0.f };
        float fov = 47.f * M_PI / 180.f; // vertical field of view.
        float aspect = 960.f / 680.f;

        AddComponent<CameraComponent>(view, up, fov, aspect, 0.1f, 10.f);
    }
}