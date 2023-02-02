#include "HEntity.h"
#include "../scene/HScene.h"
#include "HComponent.h"

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
        m_pScene       = pScene;
        m_entityHandle = handle;
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
        delete meshComponent.m_pPos;
    }

    // ================================================================================================================
    void HCubeEntity::OnDefineEntity()
    {
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
                                          nullptr,
                                          6);
    }
}