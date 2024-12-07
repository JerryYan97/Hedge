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
        virtual ~HEntity(); // We destruct the childen class casted to the parent class pointer.

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

        void GetComponentsNamesHashes(std::vector<uint32_t>& output) { output = m_componentsNamesHashes; }

        uint32_t GetEntityHandle() { return m_entityHandle; }

    protected:
        template<typename Type, typename... Args>
        void AddComponent(Args &&...args);

        template<typename T>
        T& GetComponent();
        
        virtual void InitComponentsNamesHashes() = 0;
        
        std::vector<uint32_t> m_componentsNamesHashes;

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
        
        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
        static HEntity* CreateEntity() { return new HCubeEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override;
    };

    class HCameraEntity : public HEntity
    {
    public:
        HCameraEntity()
            : HEntity("HCameraEntity", "DefaultCameraInst"),
              m_isHold(false),
              m_holdStartPos{ 0.0f, 0.0f },
              m_holdStartView{ 0.0f, 0.0f, 0.0f },
              m_holdStartUp{ 0.0f, 0.0f, 0.0f },
              m_holdRight{ 0.0f, 0.0f, 0.0f }
        {}

        ~HCameraEntity();

        virtual void OnDefineEntity(HEventManager& eventManager);
        virtual bool OnEvent(HEvent& ievent);

        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
        static HEntity* CreateEntity() { return new HCameraEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override;

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

        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
        static HEntity* CreateEntity() { return new HPointLightEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override;

    private:
    };

    class HImageBasedLightingEntity : public HEntity
    {
    public:
        HImageBasedLightingEntity() : HEntity("HImageBasedLightingEntity", "DefaultImageBasedLightingInst") {}
        ~HImageBasedLightingEntity() {}

        virtual void OnDefineEntity(HEventManager& eventManager) {}
        virtual bool OnEvent(HEvent& ievent) { return true; }

        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis) {}
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis) {}
        static HEntity* CreateEntity() { return new HImageBasedLightingEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override {};

    private:
    };
}