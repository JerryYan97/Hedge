#pragma once
#include "../core/HEntity.h"
#include "../input/InputHandler.h"

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
        virtual bool OnEvent(HEvent& ievent) override;
        
        // Seralization
        static void Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis);
        static void Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
        static HEntity* CreateEntity() { return new HMainGameEntity(); };

    protected:
        virtual void InitComponentsNamesHashes() override {}

    private:
        class BoardMoveCommandGenerator : public CommandGenerator
        {
        public:
            BoardMoveCommandGenerator()
            {
                std::unordered_set<InputEnum> camRotateKeyCombs = {PRESS_UP, PRESS_DOWN};
                SetKeyCombination(camRotateKeyCombs);
                m_isKeyCombAnd = false;
            }

            CustomizedCommand GenerateCommand(const std::vector<ImGuiInput> inputs) override
            {
                int up = 0;
                if (inputs[0].GetInputEnum() == InputEnum::PRESS_UP)
                {
                    up = 1;
                }
                else
                {
                    up = -1;
                }

                CustomizedCommand cmd;
                cmd.m_commandTypeUID = m_cmdGenCmdTypeUID;
                cmd.m_payloadInts.push_back(up);
                return cmd;
            }
        };

        BoardMoveCommandGenerator m_boardMoveCommandGenerator;
    };
}