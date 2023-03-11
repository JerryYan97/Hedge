#pragma once
#include <vector>
#include <string>
#include "UtilMath.h"

namespace YAML
{
    class Node;
    class Emitter;
}

namespace Hedge
{
    class HScene;
    class HComponent;
    class HRenderManager;
    class HEvent;
    class HEventManager;

    class HEntity
    {
    public:
        HEntity(const std::string& className, const std::string& instName);
        ~HEntity();

        // Internal used funcs
        void CreateInSceneInternal(HScene* pScene, uint32_t handle);

        virtual void OnSpawn() {};
        virtual void PreRenderTick(float dt) {};
        virtual void PostRenderTick(float dt) {};
        virtual bool OnEvent(HEvent& ievent) = 0;

        // Add components to entity to define an entity.
        virtual void OnDefineEntity(HEventManager& eventManager) = 0;

        uint32_t GetClassNameHash() { return m_entityClassNameHash; }
        std::string& GetEntityInstName() { return m_customName; }

    protected:
        template<typename Type, typename... Args>
        void AddComponent(Args &&...args);

        template<typename T>
        T& GetComponent();

        uint32_t GetEntityHandle() { return m_entityHandle; }

    private:
        uint32_t m_entityClassNameHash;
        uint32_t m_entityHandle;
        HScene*  m_pScene;
        std::string m_customName;
    };

    class HCubeEntity : public HEntity
    {
    public:
        HCubeEntity() : HEntity("HCubeEntity", "DefaultCubeInst") {};
        ~HCubeEntity();

        virtual void OnDefineEntity(HEventManager& eventManager);
        virtual bool OnEvent(HEvent& ievent) { return true; }
        
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, Hedge::HEntity* pThis) {};
    };

    class HCameraEntity : public HEntity
    {
    public:
        HCameraEntity()
            : HEntity("HCameraEntity", "DefaultCameraInst"),
              m_isHold(false)
        {};

        ~HCameraEntity();

        virtual void OnDefineEntity(HEventManager& eventManager);
        virtual bool OnEvent(HEvent& ievent);

        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, Hedge::HEntity* pThis) {};

    private:
        void OnMouseMiddleButtonEvent(HEvent& ievent);
        void OnKeyWEvent(HEvent& ievent);
        void OnKeySEvent(HEvent& ievent);
        void OnKeyAEvent(HEvent& ievent);
        void OnKeyDEvent(HEvent& ievent);

        HFVec2 m_holdStartPos;
        float m_holdStartView[3];
        float m_holdStartUp[3];
        float m_holdRight[3];
        bool m_isHold;
    };

    class HPointLightEntity : public HEntity
    {
    public:
        HPointLightEntity() : HEntity("HPointLightEntity", "DefaultPointLightInst") {}
        ~HPointLightEntity() {}

        virtual void OnDefineEntity(HEventManager& eventManager);
        virtual bool OnEvent(HEvent& ievent) { return true; }

        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, Hedge::HEntity* pThis);

    private:
    };
}