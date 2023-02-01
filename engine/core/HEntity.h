#pragma once
#include <vector>
#include <string>

namespace Hedge
{
    class HScene;
    class HComponent;

    class HEntity
    {
    public:
        HEntity();
        ~HEntity();

        // Internal used funcs
        void CreateInSceneInternal(HScene* pScene, uint32_t handle);

        virtual void OnSpawn() {};
        virtual void PreRenderTick(float dt) {};
        virtual void PostRenderTick(float dt) {};

        // Add components to entity to define an entity.
        virtual void OnDefineEntity() = 0;

    protected:
        template<typename... Args>
        void AddComponent(Args &&...args);

        template<typename T>
        T& GetComponent();

    private:
        uint32_t m_entityHandle;
        HScene*  m_pScene;
    };

    class HCubeEntity : public HEntity
    {
    public:
        HCubeEntity();
        ~HCubeEntity();

        virtual void OnDefineEntity();
        
    };
}