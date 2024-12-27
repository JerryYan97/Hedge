#include "InputHandler.h"
#include "imgui.h"

namespace Hedge
{
    uint32_t CommandGenerator::m_commandTypeUIDCounter = 0;

    // ================================================================================================================
    ImGuiInputHandler::ImGuiInputHandler()
    {
    }

    // ================================================================================================================
    ImGuiInputHandler::~ImGuiInputHandler()
    {
        m_commandGenerators.clear();
    }

    // ================================================================================================================
    void ImGuiInputHandler::AddOrUpdateCommandGenerator(
        CommandGenerator*  pCommandGenerator)
    {
        if (pCommandGenerator == nullptr)
        {
            return;
        }

        for (auto& itr : m_commandGenerators)
        {
            if (itr->GetKeyCombination() == pCommandGenerator->GetKeyCombination())
            {
                m_commandGenerators.erase(itr);
                delete itr;
                return;
            }
        }

        m_commandGenerators.insert(pCommandGenerator);
    }

    // ================================================================================================================
    std::unordered_set<InputEnum> ImGuiInputHandler::GenerateInputEnum(
        const std::vector<ImGuiInput>& inputs)
    {
        std::unordered_set<InputEnum> inputEnums;
        for (const auto& input : inputs)
        {
            inputEnums.insert(input.GetInputEnum());
        }
        return inputEnums;
    }

    // ================================================================================================================
    std::vector<CustomizedCommand> ImGuiInputHandler::HandleInput()
    {
        ImGuiIO& io = ImGui::GetIO();

        std::vector<CustomizedCommand> commands;
        std::vector<ImGuiInput>        frameInputs;

        // Common ImGUI input parameters:
        // io.MousePos, io.MouseDelta, ImGui::IsMouseDown(i), ImGui::IsKeyDown(ImGuiKey key), ...etc.
        // Cache the current frame inputs
        if(ImGui::IsKeyDown(ImGuiKey_W))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_W));
        }
        
        if (ImGui::IsKeyDown(ImGuiKey_A))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_A));
        }

        if (ImGui::IsKeyDown(ImGuiKey_S))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_S));
        }

        if (ImGui::IsKeyDown(ImGuiKey_D))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_D));
        }

        if (ImGui::IsKeyDown(ImGuiKey_R))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_R));
        }

        if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_UP));
        }

        if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_DOWN));
        }

        if (ImGui::IsKeyDown(ImGuiKey_Escape))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_ESC));
        }
        
        if(abs(io.MouseDelta.x) > 0.f || abs(io.MouseDelta.y) > 0.f)
        {
            ImGuiInput mouseMove(InputEnum::MOUSE_MOVE);
            mouseMove.AddFloat(io.MouseDelta.x);
            mouseMove.AddFloat(io.MouseDelta.y);
            mouseMove.AddFloat(io.MousePos.x);
            mouseMove.AddFloat(io.MousePos.y);

            frameInputs.push_back(mouseMove);
        }

        // Middle mouse button
        if (ImGui::IsMouseDown(2))
        {
            frameInputs.push_back(ImGuiInput(InputEnum::PRESS_MOUSE_MIDDLE_BUTTON));
        }

        std::unordered_set<InputEnum> inputEnums = GenerateInputEnum(frameInputs);

        // Generate the commands
        for (const auto& itr : m_commandGenerators)
        {
            if (itr->CheckKeyCombination(inputEnums))
            {
                commands.push_back(itr->GenerateCommand(frameInputs));
            }
        }

        return commands;
    }

    // ================================================================================================================
    bool CommandGenerator::CheckKeyCombination(
        const std::unordered_set<InputEnum>& curInputStates)
    {
        if (m_isKeyCombAnd)
        {
            for (const auto& itr : m_keycombination)
            {
                if (curInputStates.count(itr) == 0)
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            for (const auto& itr : m_keycombination)
            {
                if (curInputStates.count(itr) > 0)
                {
                    return true;
                }
            }
            return false;
        }
    }

    // ================================================================================================================
    ImGuiInput CommandGenerator::FindQualifiedInput(
        InputEnum                                iEnum,
        const std::vector<ImGuiInput> inputs)
    {
        for (const auto& itr : inputs)
        {
            if (itr.GetInputEnum() == iEnum)
            {
                return itr;
            }
        }
        return ImGuiInput(InputEnum::INVALID);
    }
}