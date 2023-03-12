#include "HComponent.h"
#include "yaml-cpp/yaml.h"

namespace Hedge
{
    // ================================================================================================================
    void TransformComponent::Seralize(
        YAML::Emitter& emitter)
    {

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "POS";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_pos[0] << m_pos[1] << m_pos[2] << YAML::EndSeq;
        emitter << YAML::EndMap;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "ROT";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_rot[0] << m_rot[1] << m_rot[2] << YAML::EndSeq;
        emitter << YAML::EndMap;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "SCALE";
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_scale[0] << m_scale[1] << m_scale[2] << YAML::EndSeq;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void StaticMeshComponent::Seralize(
        YAML::Emitter& emitter)
    {
        emitter << YAML::BeginMap;
        emitter << YAML::EndMap;
    }
}