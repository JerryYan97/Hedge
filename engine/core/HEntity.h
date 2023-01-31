#pragma once

namespace Hedge
{
    class HEntity
    {
    public:
        HEntity();
        ~HEntity();

        template<typename T>
        void AddComponent(T component);

        virtual void OnSpawn() = 0;
        virtual void PreRenderTick(float dt) = 0;
        virtual void PostRenderTick(float dt) = 0;
    private:

    };
}