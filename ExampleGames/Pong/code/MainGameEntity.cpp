#include "MainGameEntity.h"
#include "PongGame.h"
#include "Utils.h"
#include "UtilMath.h"
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
        m_ballSpeed = 2.f;
        m_ballVecDir[0] = 1.0f;
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
    void HMainGameEntity::GetBoardsHandles()
    {
        // Get the board entities handles
        if (m_playerBoardHandle == 0 || m_opponentBoardHandle == 0 || m_ballHandle == 0)
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            std::vector<std::pair<std::string, uint32_t>> entities;
            scene.GetAllEntitiesNamesHashes(entities);

            for (auto& entity : entities)
            {
                if (std::strcmp(entity.first.c_str(), "PlayerBoardInst") == 0)
                {
                    std::cout << "Player Board Entity: " << entity.first << std::endl;
                    m_playerBoardHandle = entity.second;
                }

                if (std::strcmp(entity.first.c_str(), "OpponentBoardInst") == 0)
                {
                    std::cout << "Opponent Board Entity: " << entity.first << std::endl;
                    m_opponentBoardHandle = entity.second;
                }

                if (std::strcmp(entity.first.c_str(), "PongBallInst") == 0)
                {
                    std::cout << "PongBallInst Entity: " << entity.first << std::endl;
                    m_ballHandle = entity.second;
                }

                if (m_playerBoardHandle != 0 && m_opponentBoardHandle != 0 && m_ballHandle != 0)
                {
                    break;
                }
            }
        }
    }

    // ================================================================================================================
    void HMainGameEntity::CalBoundingBox()
    {
        HScene& scene = g_pFrameListener->GetActiveScene();
        HEntity* pPlayerBoard = scene.GetEntity(m_playerBoardHandle);
        HEntity* pOpponentBoard = scene.GetEntity(m_opponentBoardHandle);

        TransformComponent& playerTransComponent = pPlayerBoard->GetComponent<TransformComponent>();
        TransformComponent& opponentTransComponent = pOpponentBoard->GetComponent<TransformComponent>();

        float modelSpaceMin[4] = { -1.f, -1.f, -1.f, 1.f };
        float modelSpaceMax[4] = {  1.f,  1.f,  1.f, 1.f };

        // Player board bounding box
        HMat4x4 playerBoardModelMat{};

        GenModelMat(playerTransComponent.m_pos,
            playerTransComponent.m_rot[2],
            playerTransComponent.m_rot[0],
            playerTransComponent.m_rot[1],
            playerTransComponent.m_scale,
            playerBoardModelMat.eles);

        MatMulVec(playerBoardModelMat.eles, modelSpaceMin, 4, m_playerBoundingBoxMin);
        MatMulVec(playerBoardModelMat.eles, modelSpaceMax, 4, m_playerBoundingBoxMax);

        // Opponent board bounding box
        HMat4x4 opponentBoardModelMat{};

        GenModelMat(opponentTransComponent.m_pos,
            opponentTransComponent.m_rot[2],
            opponentTransComponent.m_rot[0],
            opponentTransComponent.m_rot[1],
            opponentTransComponent.m_scale,
            opponentBoardModelMat.eles);

        MatMulVec(opponentBoardModelMat.eles, modelSpaceMin, 4, m_opponentBoundingBoxMin);
        MatMulVec(opponentBoardModelMat.eles, modelSpaceMax, 4, m_opponentBoundingBoxMax);
    }

    // ================================================================================================================
    void HMainGameEntity::PreRenderTick(float deltaTime)
    {
        HScene& scene = g_pFrameListener->GetActiveScene();
        GetBoardsHandles();

        HGameGuiManager* pGuiManager = dynamic_cast<HGameGuiManager*>(g_pGuiManager);
        if (pGuiManager)
        {
            pGuiManager->SetPlayerScore(m_playerScore);
            pGuiManager->SetOpponentScore(m_opponentScore);
        }

        std::cout << "Delta Time: " << std::to_string(deltaTime) << std::endl;

        // Update the ball physics
        if (m_ballHandle != 0)
        {
            if (deltaTime > 1.f)
            {
                deltaTime = 0.f;
            }

            HEntity* pBall = scene.GetEntity(m_ballHandle);
            TransformComponent& transComponent = pBall->GetComponent<TransformComponent>();

            float vel[3] = {};
            memcpy(vel, m_ballVecDir, sizeof(float) * 3);
            ScalarMul(m_ballSpeed, vel, 3);
            ScalarMul(deltaTime, vel, 3);
            VecAdd(transComponent.m_pos, vel, 3, transComponent.m_pos);

            // Update boards bounding boxes
            CalBoundingBox();

            // Change the ball direction if it hits the board
            if (transComponent.m_pos[0] > m_opponentBoundingBoxMax[0] || transComponent.m_pos[0] < m_playerBoundingBoxMax[0])
            {
                m_ballVecDir[0] *= -1.f;
            }
        }
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

                if (m_playerBoardHandle != 0)
                {
                    HScene& scene = g_pFrameListener->GetActiveScene();
                    HEntity* pPlayerBoard = scene.GetEntity(m_playerBoardHandle);
                    TransformComponent& transComponent = pPlayerBoard->GetComponent<TransformComponent>();
                    transComponent.m_pos[1] += move * 0.1f;
                }
            }
        }
        return true;
    }
}
