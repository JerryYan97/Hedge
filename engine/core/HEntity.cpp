#include "HEntity.h"
#include "../scene/HScene.h"

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
    void HEntity::CreateInSceneInternal(HScene* pScene, uint32_t handle)
    {
        m_pScene       = pScene;
        m_entityHandle = handle;
    }

    // ================================================================================================================
    template<typename... Args>
    void HEntity::AddComponent(Args &&...args)
    {
        m_pScene->EntityAddComponent(m_entityHandle, args);
    }

    // ================================================================================================================
    template<typename T>
    T& HEntity::GetComponent()
    {
        return m_pScene->EntityGetComponent(uint32_t entityHandle);
    }

    // ================================================================================================================
    void HCubeEntity::OnDefineEntity()
    {

    }
}