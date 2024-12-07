#pragma once
#include "../core/HEntity.h"

using namespace Hedge;

namespace PongGame
{
    class HMainGameEntity : public HEntity
    {
    public:
        HMainGameEntity() : HEntity("HMainGameEntity", "DefaultMainGameInst") {};
        ~HMainGameEntity();

        virtual void OnDefineEntity(HEventManager& eventManager);
        virtual void PreRenderTick(float deltaTime) override;
        virtual bool OnEvent(HEvent& ievent) { return true; }
        
        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
        static HEntity* CreateEntity() { return new HMainGameEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override;
    };
}