#include "MainGameEntity.h"
#include "PongGame.h"
#include "Utils.h"
#include "UtilMath.h"
#include <ctime>
#include <cstdlib>
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

    void HMainGameEntity::RandomGenerateBallDir()
    {
        // Seed the random number generator with the current time
        srand(time(0)); 

        // Generate a random number between 1 and 100
        int randomNum = rand() % 100 + 1;

        float verticalDir = (float)randomNum / 80.f;

        m_ballVecDir[0] = 1.0f;
        m_ballVecDir[1] = verticalDir;
        NormalizeVec(m_ballVecDir, 3);
    }

    // ================================================================================================================
    void HMainGameEntity::OnDefineEntity(HEventManager& eventManager)
    {
        g_pGuiManager->AddOrUpdateCommandGenerator(&m_boardMoveCommandGenerator);
        g_pGuiManager->AddOrUpdateCommandGenerator(&m_exitGameCommandGenerator);
        
        eventManager.RegisterListener("IMGUI_INPUT", GetEntityHandle());
        m_ballSpeed = 3.f;
        RandomGenerateBallDir();
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

                if (std::strcmp(entity.first.c_str(), "TopWallInst") == 0)
                {
                    std::cout << "Upper Wall Entity: " << entity.first << std::endl;
                    m_upperWallHandle = entity.second;
                }

                if (std::strcmp(entity.first.c_str(), "BottomWallInst") == 0)
                {
                    std::cout << "Lower Wall Entity: " << entity.first << std::endl;
                    m_lowerWallHandle = entity.second;
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
        HEntity* pUpperWall = scene.GetEntity(m_upperWallHandle);
        HEntity* pLowerWall = scene.GetEntity(m_lowerWallHandle);

        TransformComponent& playerTransComponent = pPlayerBoard->GetComponent<TransformComponent>();
        TransformComponent& opponentTransComponent = pOpponentBoard->GetComponent<TransformComponent>();
        TransformComponent& upperWallTransComponent = pUpperWall->GetComponent<TransformComponent>();
        TransformComponent& lowerWallTransComponent = pLowerWall->GetComponent<TransformComponent>();

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

        // Upper wall bounding box
        HMat4x4 upperWallModelMat{};

        GenModelMat(upperWallTransComponent.m_pos,
            upperWallTransComponent.m_rot[2],
            upperWallTransComponent.m_rot[0],
            upperWallTransComponent.m_rot[1],
            upperWallTransComponent.m_scale,
            upperWallModelMat.eles);

        MatMulVec(upperWallModelMat.eles, modelSpaceMin, 4, m_upperWallBoundingBoxMin);
        MatMulVec(upperWallModelMat.eles, modelSpaceMax, 4, m_upperWallBoundingBoxMax);

        // Lower wall bounding box
        HMat4x4 lowerWallModelMat{};

        GenModelMat(lowerWallTransComponent.m_pos,
            lowerWallTransComponent.m_rot[2],
            lowerWallTransComponent.m_rot[0],
            lowerWallTransComponent.m_rot[1],
            lowerWallTransComponent.m_scale,
            lowerWallModelMat.eles);

        MatMulVec(lowerWallModelMat.eles, modelSpaceMin, 4, m_lowerWallBoundingBoxMin);
        MatMulVec(lowerWallModelMat.eles, modelSpaceMax, 4, m_lowerWallBoundingBoxMax);
    }

    // ================================================================================================================
    bool HMainGameEntity::CheckBoardCollision()
    {
        bool hitBoard = false;

        HScene& scene = g_pFrameListener->GetActiveScene();
        HEntity* pBall = scene.GetEntity(m_ballHandle);
        TransformComponent& transComponent = pBall->GetComponent<TransformComponent>();

        if (AABBCubeSphereIntersection(m_opponentBoundingBoxMin, m_opponentBoundingBoxMax, transComponent.m_pos, 0.5f))
        {
            if (m_lastCollisionType != CollisionType::OPPONENT)
            {
                hitBoard = true;
                m_lastCollisionType = CollisionType::OPPONENT;
            }
        }

        if (AABBCubeSphereIntersection(m_playerBoundingBoxMin, m_playerBoundingBoxMax, transComponent.m_pos, 0.5f))
        {
            if (m_lastCollisionType != CollisionType::PLAYER)
            {
                hitBoard = true;
                m_lastCollisionType = CollisionType::PLAYER;
            }
        }

        return hitBoard;
    }

    // ================================================================================================================
    bool HMainGameEntity::CheckWallCollision()
    {
        bool hitWall = false;

        HScene& scene = g_pFrameListener->GetActiveScene();
        HEntity* pBall = scene.GetEntity(m_ballHandle);
        TransformComponent& transComponent = pBall->GetComponent<TransformComponent>();

        if (AABBCubeSphereIntersection(m_upperWallBoundingBoxMin, m_upperWallBoundingBoxMax, transComponent.m_pos, 0.5f))
        {
            if (m_lastCollisionType != CollisionType::UPPER_WALL)
            {
                hitWall = true;
                m_lastCollisionType = CollisionType::UPPER_WALL;
            }
        }

        if (AABBCubeSphereIntersection(m_lowerWallBoundingBoxMin, m_lowerWallBoundingBoxMax, transComponent.m_pos, 0.5f))
        {
            if (m_lastCollisionType != CollisionType::LOWER_WALL)
            {
                hitWall = true;
                m_lastCollisionType = CollisionType::LOWER_WALL;
            }
        }

        return hitWall;
    }

    // ================================================================================================================
    bool HMainGameEntity::CheckOutOfBound(bool& oIsPlayerWin)
    {
        HScene& scene = g_pFrameListener->GetActiveScene();
        HEntity* pBall = scene.GetEntity(m_ballHandle);
        TransformComponent& transComponent = pBall->GetComponent<TransformComponent>();
        
        oIsPlayerWin = false;

        if (transComponent.m_pos[0] > 12.f ||
            transComponent.m_pos[0] < -12.f ||
            transComponent.m_pos[1] > 8.f ||
            transComponent.m_pos[1] < -8.f)
        {
            if (transComponent.m_pos[0] > 0.f)
            {
                oIsPlayerWin = true;
            }
            else
            {
                oIsPlayerWin = false;
            }

            return true;
        }

        return false;
    }

    // ================================================================================================================
    void HMainGameEntity::ResetTurn()
    {
        m_ballSpeed = 2.f;
        m_ballVecDir[0] = 1.0f;
        m_ballVecDir[1] = 0.0f;

        HScene& scene = g_pFrameListener->GetActiveScene();
        HEntity* pBall = scene.GetEntity(m_ballHandle);
        TransformComponent& transComponent = pBall->GetComponent<TransformComponent>();
        transComponent.m_pos[0] = 0.f;
        transComponent.m_pos[1] = 0.f;

        HEntity* pPlayerBoard = scene.GetEntity(m_playerBoardHandle);
        TransformComponent& playerTransComponent = pPlayerBoard->GetComponent<TransformComponent>();
        playerTransComponent.m_pos[1] = 0.f;

        HEntity* pOpponentBoard = scene.GetEntity(m_opponentBoardHandle);
        TransformComponent& opponentTransComponent = pOpponentBoard->GetComponent<TransformComponent>();
        opponentTransComponent.m_pos[1] = 0.f;

        m_lastCollisionType = CollisionType::NONE;

        RandomGenerateBallDir();
    }

    // ================================================================================================================
    void HMainGameEntity::PreRenderTick(float deltaTime)
    {
        if (m_pauseGame)
        {
            return;
        }

        HGameGuiManager* pGuiManager = dynamic_cast<HGameGuiManager*>(g_pGuiManager);
        if (pGuiManager)
        {
            pGuiManager->SetPlayerScore(m_playerScore);
            pGuiManager->SetOpponentScore(m_opponentScore);
        }

        HScene& scene = g_pFrameListener->GetActiveScene();
        GetBoardsHandles();

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
            if (CheckBoardCollision())
            {
                m_ballVecDir[0] *= -1.f;
                m_ballSpeed *= 1.5f;
                m_ballSpeed = std::clamp(m_ballSpeed, 0.f, 15.f);
            }

            if (CheckWallCollision())
            {
                m_ballVecDir[1] *= -1.f;
            }
        }

        // Update the opponent board movement
        AiMovement(deltaTime);

        // Check if the ball is out of bounds. If so, reset the ball position and update the score board.
        if (m_ballHandle != 0)
        {
            bool isPlayerWin = false;
            if (CheckOutOfBound(isPlayerWin))
            {
                if (isPlayerWin)
                {
                    m_playerScore++;
                }
                else
                {
                    m_opponentScore++;
                }

                ResetTurn();

                if (m_playerScore == 1 || m_opponentScore == 1)
                {
                    if (pGuiManager)
                    {
                        pGuiManager->SetPlayerScore(m_playerScore);
                        pGuiManager->SetOpponentScore(m_opponentScore);
                        pGuiManager->ShowPauseGameGui(m_playerScore == 1);
                    }

                    g_pGuiManager->AddOrUpdateCommandGenerator(&m_restartGameCommandGenerator);

                    m_playerScore = 0;
                    m_opponentScore = 0;

                    HEventArguments roundEndArgs;
                    roundEndArgs[crc32("PLAYER_WIN")] = isPlayerWin;
                    HEvent roundEndEvent(roundEndArgs, "ROUND_END");

                    HPongGame* pPongGame = dynamic_cast<HPongGame*>(g_pFrameListener);
                    HScene* scene = pPongGame->GetActiveScenePtr();
                    m_pauseGame = true;
                    g_pFrameListener->GetEventManager().SendEvent(roundEndEvent, scene);
                }
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
                if (m_pauseGame)
                {
                    return true;
                }

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

            if (cmdType == m_exitGameCommandGenerator.GetCmdTypeUID())
            {
                g_pFrameListener->SetCloseGame();
            }

            if (cmdType == m_restartGameCommandGenerator.GetCmdTypeUID())
            {
                m_pauseGame = false;
                HGameGuiManager* pGuiManager = dynamic_cast<HGameGuiManager*>(g_pGuiManager);
                pGuiManager->HidePauseGameGui();
                g_pGuiManager->RemoveCommandGenerator(&m_restartGameCommandGenerator);
            }
        }
        return true;
    }

    // ================================================================================================================
    void HMainGameEntity::AiMovement(float deltaTime)
    {
        const float AI_SPEED_FACTOR = 3.f;

        if (m_opponentBoardHandle != 0)
        {
            HScene& scene = g_pFrameListener->GetActiveScene();
            HEntity* pOpponentBoard = scene.GetEntity(m_opponentBoardHandle);
            TransformComponent& transComponent = pOpponentBoard->GetComponent<TransformComponent>();

            if (m_ballHandle != 0)
            {
                HEntity* pBall = scene.GetEntity(m_ballHandle);
                TransformComponent& ballTransComponent = pBall->GetComponent<TransformComponent>();

                float dist = std::abs(ballTransComponent.m_pos[1] - transComponent.m_pos[1]);

                float offset = AI_SPEED_FACTOR * deltaTime * dist;

                if (ballTransComponent.m_pos[1] > transComponent.m_pos[1])
                {
                    transComponent.m_pos[1] += offset;
                }
                else if (ballTransComponent.m_pos[1] < transComponent.m_pos[1])
                {
                    transComponent.m_pos[1] -= offset;
                }
            }
        }
    }
}
