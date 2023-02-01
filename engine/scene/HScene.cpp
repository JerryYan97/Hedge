#include "HScene.h"
#include "../core/HEntity.h"

namespace Hedge
{
    // ================================================================================================================
    HScene::HScene()
    {}

    // ================================================================================================================
    HScene::~HScene()
    {

    }

    // ================================================================================================================
    void HScene::SpawnEntity(HEntity* pEntity)
    {
        entt::entity newEntity = m_registry.create();
        pEntity->CreateInSceneInternal(this, static_cast<uint32_t>(newEntity));

        pEntity->OnDefineEntity();
        // m_registry.create
    }

    // ================================================================================================================
    template<typename... Args>
    void HScene::EntityAddComponent(uint32_t entityHandle, Args &&...args)
    {
        m_registry.emplace<T>(entityHandle, std::forward<Args>(args)...);
    }

    // ================================================================================================================
    template<typename T>
    T& HScene::EntityGetComponent(uint32_t entityHandle)
    {
        return m_registry<T>(entityHandle);
    }
}