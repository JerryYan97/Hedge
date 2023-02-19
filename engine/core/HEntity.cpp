#include "HEntity.h"
#include "../scene/HScene.h"
#include "../render/HRenderManager.h"
#include "HComponent.h"

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
    void HCubeEntity::OnDefineEntity()
    {
        //
        float pos[3] = {0.f, 0.f, 0.f};
        float rot[4] = {0.f, 0.f, 0.f, 0.f};
        float scale[3] = {1.f, 1.f, 1.f};

        AddComponent<TransformComponent>(pos, rot, scale);

        // pos1, pos2, pos3, col1, col2, col3
        float* verts = new float[] {
                -0.75f, -0.75f, 0.f, 1.f, 0.f, 0.f, // v0 - Top Left
                0.75f, -0.75f, 0.f, 0.f, 1.f, 0.f, // v1 - Top Right
                0.75f, 0.75f, 0.f, 0.f, 0.f, 1.f, // v2 - Bottom Right
                -0.75f, 0.75f, 0.f, 1.f, 1.f, 0.f // v3 - Bottom Left
        };

        // CCW
        // v0 - v1 - v2; v2 - v3 - v0;
        uint32_t* vertIdx = new uint32_t[] {
            0, 1, 2, 2, 3, 0
        };

        AddComponent<StaticMeshComponent>(vertIdx,
                                          verts,
                                          6);
    }

    // ================================================================================================================
    HCameraEntity::~HCameraEntity()
    {}

    // ================================================================================================================
    void HCameraEntity::OnDefineEntity()
    {
        float pos[3] = { 0.f, 0.f, -1.f };
        float rot[4] = { 0.f, 0.f, 0.f, 0.f };
        float scale[3] = { 1.f, 1.f, 1.f };

        AddComponent<TransformComponent>(pos, rot, scale);

        float view[3] = { 0.f, 0.f, 1.f };
        float up[3] = { 0.f, 1.f, 0.f };
        float fov = 60.f * M_PI / 180.f;
        float aspect = 960.f / 680.f;

        AddComponent<CameraComponent>(view, up, fov, aspect, 0.1f, 10.f);
    }
}