#include "HEntity.h"
#include "../scene/HScene.h"
#include "../render/HRenderManager.h"
#include "HComponent.h"
#include "Utils.h"
#include "HEvent.h"
#include "../core/HAssetRsrcManager.h"

#include "yaml-cpp/yaml.h"

#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

extern Hedge::HAssetRsrcManager* g_pAssetRsrcManager;

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
        g_pAssetRsrcManager->ReleaseAsset(meshComponent.m_meshAssetGuid);
    }

    // ================================================================================================================
    void HCubeEntity::OnDefineEntity(
        HEventManager& eventManager)
    {
        float pos[3] = {0.f, 0.f, 2.f};
        float rot[3] = {0.f, 0.f, 0.f};
        float scale[3] = {1.f, 1.f, 1.f};

        AddComponent<TransformComponent>(pos, rot, scale);
        AddComponent<StaticMeshComponent>();
    }

    // ================================================================================================================
    // TODO: Component needs to be class and has its own serialize and deserialize operations.
    void HCubeEntity::Seralize(
        YAML::Emitter& emitter,
        Hedge::HEntity* pThis)
    {
        HCubeEntity* pCubeEntity = dynamic_cast<HCubeEntity*>(pThis);

        emitter << YAML::Key << pCubeEntity->GetEntityInstName();
        emitter << YAML::Value;

        {
            // Entity Type
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Type";
            emitter << YAML::Value << "HCubeEntity";
            emitter << YAML::Key << "Components";
            emitter << YAML::Value;
            {
                emitter << YAML::BeginMap;
                // Transform Component
                TransformComponent& transComponent = pCubeEntity->GetComponent<TransformComponent>();
                transComponent.Seralize(emitter);

                // Static Mesh Component
                StaticMeshComponent& meshComponent = pCubeEntity->GetComponent<StaticMeshComponent>();
                meshComponent.Seralize(emitter);
                emitter << YAML::EndMap;
            }
            emitter << YAML::EndMap;
        }
    }

    // ================================================================================================================
    void HCubeEntity::Deseralize(
        YAML::Node& node,
        const std::string& name,
        Hedge::HEntity* pThis)
    {
        HCubeEntity* pCubeEntity = dynamic_cast<HCubeEntity*>(pThis);

        // Transform Component
        TransformComponent& transComponent = pCubeEntity->GetComponent<TransformComponent>();
        transComponent.Deseralize(node["TransformComponent"]);

        // Static Mesh Component
        StaticMeshComponent& meshComponent = pCubeEntity->GetComponent<StaticMeshComponent>();
        meshComponent.Deseralize(node["StaticMeshComponent"]);
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

        NormalizeVec(view, 3); // NOTE: It looks like it can be deleted.
        float right[3] = {};
        CrossProductVec3(view, up, right);
        NormalizeVec(right, 3);
        CrossProductVec3(right, view, up); // NOTE: It looks like it can be deleted.
        NormalizeVec(up, 3); // NOTE: It looks like it can be deleted.

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
        
        emitter << YAML::Key << pCameraEntity->GetEntityInstName();
        emitter << YAML::Value;

        {
            // Entity Type
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Type";
            emitter << YAML::Value << "HCameraEntity";
            emitter << YAML::Key << "Components";
            emitter << YAML::Value;
            {
                emitter << YAML::BeginMap;
                // Transform Component
                TransformComponent& transComponent = pCameraEntity->GetComponent<TransformComponent>();
                transComponent.Seralize(emitter);

                // Camera Component
                CameraComponent& camComponent = pCameraEntity->GetComponent<CameraComponent>();
                camComponent.Seralize(emitter);
                emitter << YAML::EndMap;
            }
            emitter << YAML::EndMap;
        }
    }

    // ================================================================================================================
    void HCameraEntity::Deseralize(
        YAML::Node& node,
        const std::string& name,
        Hedge::HEntity* pThis)
    {
        HCameraEntity* pCameraEntity = dynamic_cast<HCameraEntity*>(pThis);

        // Transform Component
        auto& transComponent = pCameraEntity->GetComponent<TransformComponent>();
        transComponent.Deseralize(node["TransformComponent"]);

        // Camera Component
        auto& camComponent = pCameraEntity->GetComponent<CameraComponent>();
        camComponent.Deseralize(node["CameraComponent"]);
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

        emitter << YAML::Key << pPtLightEntity->GetEntityInstName();
        emitter << YAML::Value;

        {
            // Entity Type
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Type";
            emitter << YAML::Value << "HPointLightEntity";
            emitter << YAML::Key << "Components";
            emitter << YAML::Value;
            {
                emitter << YAML::BeginMap;
                // Transform Component
                TransformComponent& transComponent = pPtLightEntity->GetComponent<TransformComponent>();
                transComponent.Seralize(emitter);
                emitter << YAML::EndMap;
            }            
            emitter << YAML::EndMap;
        }
    }

    // ================================================================================================================
    void HPointLightEntity::Deseralize(
        YAML::Node& node,
        const std::string& name,
        Hedge::HEntity* pThis)
    {
        HPointLightEntity* pPtLightEntity = dynamic_cast<HPointLightEntity*>(pThis);

        // Transform Component
        auto& transComponent = pPtLightEntity->GetComponent<TransformComponent>();
        transComponent.Deseralize(node["TransformComponent"]);

        // Point light Component
        auto& pointLightComponent = pPtLightEntity->GetComponent<PointLightComponent>();
        pointLightComponent.Deseralize(node["PointLightComponent"]);
    }
}