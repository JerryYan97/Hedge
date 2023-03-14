#include "HComponent.h"
#include "yaml-cpp/yaml.h"

namespace Hedge
{
    extern float CubeVertBufData[192];
    extern uint32_t CubeIdxData[36];

    // ================================================================================================================
    void TransformComponent::Seralize(
        YAML::Emitter& emitter)
    {
        emitter << YAML::Key << "TransformComponent";
        emitter << YAML::Value;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "POS";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_pos[0] << m_pos[1] << m_pos[2] << YAML::EndSeq;

        emitter << YAML::Key << "ROT";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_rot[0] << m_rot[1] << m_rot[2] << YAML::EndSeq;

        emitter << YAML::Key << "SCALE";
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_scale[0] << m_scale[1] << m_scale[2] << YAML::EndSeq;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void TransformComponent::Deseralize(
        YAML::Node& node)
    {
        std::vector<float> pos = node["POS"].as<std::vector<float>>();
        std::vector<float> rot = node["ROT"].as<std::vector<float>>();
        std::vector<float> scale = node["SCALE"].as<std::vector<float>>();
        
        memcpy(m_pos, pos.data(), 3 * sizeof(float));
        memcpy(m_rot, rot.data(), 3 * sizeof(float));
        memcpy(m_scale, scale.data(), 3 * sizeof(float));
    };

    // ================================================================================================================
    void StaticMeshComponent::Seralize(
        YAML::Emitter& emitter)
    {
        emitter << YAML::Key << "StaticMeshComponent";
        emitter << YAML::Value;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "Is Prebuilt";
        emitter << YAML::Value << m_preBuiltMesh;

        if (m_preBuiltMesh)
        {
            emitter << YAML::Key << "Prebuilt Mesh Name";
        }
        else
        {
            emitter << YAML::Key << "Mesh Asset Name Path";
        }
        emitter << YAML::Value << m_meshAssetPathName;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void StaticMeshComponent::Deseralize(
        YAML::Node& node)
    {
        bool isPrebuiltMesh = node["Is Prebuilt"].as<bool>();

        if (isPrebuiltMesh)
        {
            // TODO: We may want to make a jump table for the prebuilt meshes.
            std::string meshName = node["Prebuilt Mesh Name"].as<std::string>();
            if (std::strcmp(meshName.c_str(), "Cube"))
            {
                m_pVert = CubeVertBufData;
                m_pIdx = CubeIdxData;
                m_idxCnt = uint32_t(sizeof(CubeIdxData) / sizeof(uint32_t));
                m_vertBufBytes = uint32_t(sizeof(CubeVertBufData));
                m_meshAssetPathName = "Cube";
                m_preBuiltMesh = true;
            }
        }
    }

    // ================================================================================================================
    void CameraComponent::Seralize(
        YAML::Emitter& emitter)
    {
        emitter << YAML::Key << "CameraComponent";
        emitter << YAML::Value;

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "View";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_view[0] << m_view[1] << m_view[2] << YAML::EndSeq;

        emitter << YAML::Key << "Up";
        emitter << YAML::Value;
        emitter << YAML::Flow;
        emitter << YAML::BeginSeq << m_up[0] << m_up[1] << m_up[2] << YAML::EndSeq;

        emitter << YAML::Key << "fov";
        emitter << YAML::Value << m_fov;

        emitter << YAML::Key << "aspect";
        emitter << YAML::Value << m_aspect;

        emitter << YAML::Key << "far";
        emitter << YAML::Value << m_far;

        emitter << YAML::Key << "near";
        emitter << YAML::Value << m_near;

        emitter << YAML::Key << "isActive";
        emitter << YAML::Value << m_active;
        emitter << YAML::EndMap;
    }

    // ================================================================================================================
    void CameraComponent::Deseralize(
        YAML::Node& node)
    {
        std::vector<float> view = node["View"].as<std::vector<float>>();
        std::vector<float> up = node["Up"].as<std::vector<float>>();
        float fov = node["fov"].as<float>();
        float aspect = node["aspect"].as<float>();
        float far = node["far"].as<float>();
        float near = node["near"].as<float>();
        bool isActive = node["isActive"].as<bool>();

        memcpy(m_view, view.data(), 3 * sizeof(float));
        memcpy(m_up, up.data(), 3 * sizeof(float));
        m_fov = fov;
        m_aspect = aspect;
        m_far = far;
        m_near = near;
        m_active = isActive;
    }
}