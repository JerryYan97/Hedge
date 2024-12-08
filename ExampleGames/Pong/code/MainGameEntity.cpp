#include "MainGameEntity.h"
#include "PongGame.h"
#include "Utils.h"
#include "../core/HComponent.h"

extern Hedge::HBaseGuiManager* g_pGuiManager;
extern Hedge::HFrameListener* g_pFrameListener;

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
        HMainGameEntity* pMainGameEntity = dynamic_cast<HMainGameEntity*>(pThis);
        pMainGameEntity->m_customName = name;
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

                HScene& scene = g_pFrameListener->GetActiveScene();

                std::vector<std::pair<std::string, uint32_t>> entities;
                scene.GetAllEntitiesNamesHashes(entities);

                uint32_t playerBoardHandle = 0;
                for (auto& entity : entities)
                {
                    if (std::strcmp(entity.first.c_str(), "PlayerBoardInst") == 0)
                    {
                        std::cout << "Entity: " << entity.first << std::endl;
                        playerBoardHandle = entity.second;
                        break;
                    }
                }
                HEntity* pPlayerBoard = scene.GetEntity(playerBoardHandle);
                TransformComponent& transComponent = pPlayerBoard->GetComponent<TransformComponent>();
                transComponent.m_pos[1] += move * 0.1f;
                // scene.GetEntity();
            }
        }
        return true;
    }
}
