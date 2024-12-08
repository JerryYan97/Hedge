#include "MainGameHUDEntity.h"
#include "PongGame.h"
#include "Utils.h"

extern Hedge::HGameGuiManager* g_pGuiManager;

namespace PongGame
{
    // ================================================================================================================
    HMainGameEntity::~HMainGameEntity()
    {
        if (g_pGuiManager) {g_pGuiManager->RemoveCommandGenerator(&m_boardMoveCommandGenerator);}
    }

    // ================================================================================================================
    void HMainGameEntity::OnDefineEntity(HEventManager& eventManager)
    {
        g_pGuiManager->AddOrUpdateCommandGenerator(&m_boardMoveCommandGenerator);
        eventManager.RegisterListener("IMGUI_INPUT", GetEntityHandle());
    }

    // ================================================================================================================
    void HMainGameEntity::Seralize(YAML::Emitter& emitter, Hedge::HEntity* pThis)
    {

    }

    // ================================================================================================================
    void HMainGameEntity::Deseralize(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis)
    {

    }

    // ================================================================================================================
    void HMainGameEntity::PreRenderTick(float deltaTime)
    {

    }

    // ================================================================================================================
    bool HMainGameEntity::OnEvent(HEvent& ievent)
    {
        if (ievent.GetEventType() == crc32("IMGUI_INPUT")) {
            HEventArguments& args = ievent.GetArgs();
            uint32_t cmdType = std::any_cast<uint32_t>(args[crc32("CMD_TYPE")]);
            if (cmdType == m_boardMoveCommandGenerator.GetCmdTypeUID())
            {
                int move = std::any_cast<int>(args[crc32("INT_0")]);
                std::cout << "Board move command received: " << std::to_string(move) << std::endl;
            }
        }
        return true;
    }
}
