#include "HEntity.h"
#include "../scene/HScene.h"
#include "../render/HRenderManager.h"
#include "HComponent.h"
#include "Utils.h"
#include "HEvent.h"

#include "yaml-cpp/yaml.h"

#include "g_builtInModels.h"
#include <sstream>

// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Hedge
{
    // TODO: We can delete m_pScene's pointer reference in the HEntity since it is only used in the AddComponent and
    // GetComponent. GetComponent is not used and AddComponent is only used in OnDefineEntity(). This is only used in
    // scene's spawn entity function. So, we can change the OnDefineEntity() to OnDefineEntity(pScene) function.
    // ================================================================================================================
    HEntity::HEntity(
        const std::string& className, 
        const std::string& instName)
        : m_pScene(nullptr),
          m_entityHandle(0),
          m_entityClassNameHash(crc32(className.c_str())),
          m_customName(instName)
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
    /*
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
    */

    // TODO: Need to store the built-in geometry to real block of memory.
    // ================================================================================================================
    void HCubeEntity::OnDefineEntity(
        HEventManager& eventManager)
    {
        //
        float pos[3] = {0.f, 0.f, 2.f};
        float rot[3] = {0.f, 0.f, 0.f};
        float scale[3] = {1.f, 1.f, 1.f};

        AddComponent<TransformComponent>(pos, rot, scale);

        /*
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
        */

        AddComponent<StaticMeshComponent>(CubeIdxData,
                                          CubeVertBufData,
                                          uint32_t(sizeof(CubeIdxData) / sizeof(uint32_t)),
                                          uint32_t(sizeof(CubeVertBufData)),
                                          "Cube", true);
    }

    // ================================================================================================================
    // TODO: Component needs to be class and has its own serialize and deserialize operations.
    void HCubeEntity::Seralize(
        YAML::Emitter& emitter,
        Hedge::HEntity* pThis)
    {
        HCubeEntity* pCubeEntity = dynamic_cast<HCubeEntity*>(pThis);

        emitter << YAML::BeginMap;
        emitter << YAML::Key << pCubeEntity->GetEntityInstName();
        emitter << YAML::Value << YAML::BeginSeq;

        // Entity Type
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "Type";
        emitter << YAML::Value << "HCubeEntity";
        emitter << YAML::EndMap;

        // Transform Component
        TransformComponent& transComponent = pCubeEntity->GetComponent<TransformComponent>();
        transComponent.Seralize(emitter);

        // Static Mesh Component
        StaticMeshComponent& meshComponent = pCubeEntity->GetComponent<StaticMeshComponent>();
        meshComponent.Seralize(emitter);

        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    HCameraEntity::~HCameraEntity()
    {}

    // ================================================================================================================
    void HCameraEntity::OnDefineEntity(
        HEventManager& eventManager)
    {
        float pos[3] = { 0.f, 0.f, -1.f };
        float rot[3] = { 0.f, 0.f, 0.f};
        float scale[3] = { 1.f, 1.f, 1.f };

        AddComponent<TransformComponent>(pos, rot, scale);

        float view[3] = { 0.f, 0.f, 1.f };
        float up[3] = { 0.f, 1.f, 0.f };
        float fov = 47.f * M_PI / 180.f; // vertical field of view.
        float aspect = 960.f / 680.f;

        NormalizeVec(view, 3);
        float right[3] = {};
        CrossProductVec3(view, up, right);
        NormalizeVec(right, 3);
        CrossProductVec3(right, view, up);
        NormalizeVec(up, 3);

        AddComponent<CameraComponent>(view, up, fov, aspect, 0.1f, 100.f);

        eventManager.RegisterListener("MOUSE_MIDDLE_BUTTON", GetEntityHandle());
        eventManager.RegisterListener("KEY_W", GetEntityHandle());
        eventManager.RegisterListener("KEY_S", GetEntityHandle());
        eventManager.RegisterListener("KEY_A", GetEntityHandle());
        eventManager.RegisterListener("KEY_D", GetEntityHandle());
    }

    // ================================================================================================================
    void HCameraEntity::OnMouseMiddleButtonEvent(
        HEvent& ievent)
    {
        HEventArguments& args = ievent.GetArgs();
        bool isDown = std::any_cast<bool>(args[crc32("IS_DOWN")]);
        if (isDown)
        {
            if (m_isHold)
            {
                // Continues holding:
                // UP-Down -- Pitch; Left-Right -- Head;
                HFVec2 curPos = std::any_cast<HFVec2>(args[crc32("POS")]);
                CameraComponent& cam = GetComponent<CameraComponent>();

                float xOffset = -(curPos.ele[0] - m_holdStartPos.ele[0]);
                float yOffset = -(curPos.ele[1] - m_holdStartPos.ele[1]);

                float pitchRadien = 0.5f * yOffset * M_PI / 180.f;
                float headRadien = 0.5f * xOffset * M_PI / 180.f;

                // 1.3 model:
                float pitchRotMat[9] = {};
                GenRotationMatArb(m_holdRight, pitchRadien, pitchRotMat);

                float headRotMat[9] = {};
                float worldUp[3] = { 0.f, 1.f, 0.f };
                GenRotationMatArb(worldUp, headRadien, headRotMat);

                float rotMat[9] = {};
                MatMulMat(headRotMat, pitchRotMat, rotMat, 3);

                float newView[3];
                MatMulVec(rotMat, m_holdStartView, 3, newView);

                float newUp[3];
                MatMulVec(rotMat, m_holdStartUp, 3, newUp);

                NormalizeVec(newView, 3);
                NormalizeVec(newUp, 3);

                memcpy(cam.m_view, newView, 3 * sizeof(float));
                memcpy(cam.m_up, newUp, 3 * sizeof(float));
            }
            else
            {
                // First hold:
                m_holdStartPos = std::any_cast<HFVec2>(args[crc32("POS")]);
                CameraComponent& cam = GetComponent<CameraComponent>();
                memcpy(m_holdStartView, cam.m_view, 3 * sizeof(float));
                memcpy(m_holdStartUp, cam.m_up, 3 * sizeof(float));

                CrossProductVec3(cam.m_view, cam.m_up, m_holdRight);
                NormalizeVec(m_holdRight, 3);
            }
        }

        m_isHold = isDown;
    }

    // ================================================================================================================
    void HCameraEntity::OnKeyWEvent(
        HEvent& ievent)
    {
        HEventArguments& args = ievent.GetArgs();
        bool isDown = std::any_cast<bool>(args[crc32("IS_DOWN")]);
        if (isDown)
        {
            TransformComponent& trans = GetComponent<TransformComponent>();
            CameraComponent& cam = GetComponent<CameraComponent>();
            float moveOffset[3] = {};
            memcpy(moveOffset, cam.m_view, 3 * sizeof(float));

            ScalarMul(0.1f, moveOffset, 3);

            VecAdd(moveOffset, trans.m_pos, 3, trans.m_pos);
        }
    }

    // ================================================================================================================
    void HCameraEntity::OnKeySEvent(
        HEvent& ievent)
    {
        HEventArguments& args = ievent.GetArgs();
        bool isDown = std::any_cast<bool>(args[crc32("IS_DOWN")]);
        if (isDown)
        {
            TransformComponent& trans = GetComponent<TransformComponent>();
            CameraComponent& cam = GetComponent<CameraComponent>();
            float moveOffset[3] = {};
            memcpy(moveOffset, cam.m_view, 3 * sizeof(float));

            ScalarMul(-0.1f, moveOffset, 3);

            VecAdd(moveOffset, trans.m_pos, 3, trans.m_pos);
        }
    }

    // ================================================================================================================
    void HCameraEntity::OnKeyAEvent(
        HEvent& ievent)
    {
        HEventArguments& args = ievent.GetArgs();
        bool isDown = std::any_cast<bool>(args[crc32("IS_DOWN")]);
        if (isDown)
        {
            TransformComponent& trans = GetComponent<TransformComponent>();
            CameraComponent& cam = GetComponent<CameraComponent>();

            float right[3] = {};
            CrossProductVec3(cam.m_view, cam.m_up, right);

            ScalarMul(-0.1f, right, 3);

            VecAdd(right, trans.m_pos, 3, trans.m_pos);
        }
    }

    // ================================================================================================================
    void HCameraEntity::OnKeyDEvent(
        HEvent& ievent)
    {
        HEventArguments& args = ievent.GetArgs();
        bool isDown = std::any_cast<bool>(args[crc32("IS_DOWN")]);
        if (isDown)
        {
            TransformComponent& trans = GetComponent<TransformComponent>();
            CameraComponent& cam = GetComponent<CameraComponent>();

            float right[3] = {};
            CrossProductVec3(cam.m_view, cam.m_up, right);

            ScalarMul(0.1f, right, 3);

            VecAdd(right, trans.m_pos, 3, trans.m_pos);
        }
    }

    // ================================================================================================================
    bool HCameraEntity::OnEvent(
        HEvent& ievent)
    {
        switch (ievent.GetEventType()) {
        case crc32("MOUSE_MIDDLE_BUTTON"):
            OnMouseMiddleButtonEvent(ievent);
            break;
        case crc32("KEY_W"):
            OnKeyWEvent(ievent);
            break;
        case crc32("KEY_S"):
            OnKeySEvent(ievent);
            break;
        case crc32("KEY_A"):
            OnKeyAEvent(ievent);
            break;
        case crc32("KEY_D"):
            OnKeyDEvent(ievent);
            break;
        default:
            return false;
        }
        return true;
    }

    // ================================================================================================================
    void HCameraEntity::Seralize(
        YAML::Emitter& emitter,
        Hedge::HEntity* pThis)
    {
        HCameraEntity* pCameraEntity = dynamic_cast<HCameraEntity*>(pThis);

        emitter << YAML::BeginMap;
        emitter << YAML::Key << pCameraEntity->GetEntityInstName();
        emitter << YAML::Value << YAML::BeginSeq;

        // Entity Type
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "Type";
        emitter << YAML::Value << "HCameraEntity";
        emitter << YAML::EndMap;

        // Transform Component
        TransformComponent& transComponent = pCameraEntity->GetComponent<TransformComponent>();
        transComponent.Seralize(emitter);

        // Camera Component

        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void HPointLightEntity::OnDefineEntity(
        HEventManager& eventManager)
    {

    }

    // ================================================================================================================
    void HPointLightEntity::Seralize(
        YAML::Emitter& emitter,
        Hedge::HEntity* pThis)
    {
        HPointLightEntity* pPtLightEntity = dynamic_cast<HPointLightEntity*>(pThis);

        emitter << YAML::BeginMap;
        emitter << YAML::Key << pPtLightEntity->GetEntityInstName();
        emitter << YAML::Value << YAML::BeginSeq;

        // Entity Type
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "Type";
        emitter << YAML::Value << "HPointLightEntity";
        emitter << YAML::EndMap;

        // Transform Component
        TransformComponent& transComponent = pPtLightEntity->GetComponent<TransformComponent>();
        transComponent.Seralize(emitter);

        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void HPointLightEntity::Deseralize(
        YAML::Node& node,
        Hedge::HEntity* pThis)
    {

    }
}