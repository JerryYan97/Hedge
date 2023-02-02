#include "HScene.h"
#include "../core/HEntity.h"
#include "../core/HComponent.h"

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
        uint32_t entityHandle = static_cast<uint32_t>(newEntity);
        pEntity->CreateInSceneInternal(this, entityHandle);

        pEntity->OnDefineEntity();
        
        m_entitiesHashTable.insert({ entityHandle, pEntity });
    }

    // ================================================================================================================
    /*
    template<typename Type, typename... Args>
    void HScene::EntityAddComponent(uint32_t entityHandle, Args &&...args)
    {
        m_registry.emplace<Type>(entityHandle, std::forward<Args>(args)...);
    }
    */

    // ================================================================================================================
    /*
    template<typename T>
    T& HScene::EntityGetComponent(uint32_t entityHandle)
    {
        return m_registry.get<T>(entityHandle);
    }
    */

    // ================================================================================================================
    SceneRenderInfo HScene::GetSceneRenderInfo() const
    {
        SceneRenderInfo renderInfo{};
        auto view = m_registry.view<StaticMeshComponent>();
        for (auto entity : view)
        {
            auto& meshComponent  = view.get<StaticMeshComponent>(entity);
            renderInfo.m_pIdx    = meshComponent.m_pIdx;
            renderInfo.m_pPos    = meshComponent.m_pPos;
            renderInfo.m_pUv     = meshComponent.m_pUv;
            renderInfo.m_vertCnt = meshComponent.m_vertCnt;

            return renderInfo;
        }
    }
}