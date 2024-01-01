#pragma once
#include <cstdint>
#include <string>

namespace YAML
{
    class Node;
    class Emitter;
}

namespace Hedge
{
    // The component design should mimic the UE.
    // Component is just an attachment for entities.
    // Large data should be stored in asset and shared by different components.
    //
    // Cited from the Game Engine Architecture:
    // A component provides a single, well-defined service.
    // Some components correspond directly to a single engine subsystem, such as rendering, animation, collision, physics, audio, etc.
    // E.g. StaticMeshComponent provide all information for renderer to render the triangles out.

    // x -- pitch, y -- head, z -- roll; m_rot[pitch, head, roll].
    class TransformComponent
    {
    public:
        TransformComponent(
            float* pPos, 
            float* pRot, 
            float* pScale)
        {
            memcpy(m_pos, pPos, 3 * sizeof(float));
            memcpy(m_rot, pRot, 3 * sizeof(float));
            memcpy(m_scale, pScale, 3 * sizeof(float));
        }

        void Seralize(YAML::Emitter& emitter);
        void Deseralize(YAML::Node& node);

        float m_pos[3];
        float m_rot[3];
        float m_scale[3];
    };

    // Static mesh component just provide a slot/reference to a static mesh asset.
    class StaticMeshComponent
    {
    public:
        StaticMeshComponent()
            : m_meshAssetGuid(0)
        {}

        ~StaticMeshComponent()
        {}

        void Seralize(YAML::Emitter& emitter);
        void Deseralize(YAML::Node& node);

        std::string m_meshAssetPathName;
        uint64_t    m_meshAssetGuid;
    };

    class CameraComponent
    {
    public:
        CameraComponent(
            float* pView,
            float* pUp,
            float  fov,
            float  aspect,
            float near,
            float far)
            : m_active(true),
              m_far(far),
              m_near(near)
        {
            memcpy(m_view, pView, 3 * sizeof(float));
            memcpy(m_up, pUp, 3 * sizeof(float));
            m_fov = fov;
            m_aspect = aspect;
        }

        void Seralize(YAML::Emitter& emitter);
        void Deseralize(YAML::Node& node);

        float m_view[3];
        float m_up[3];
        float m_fov;
        float m_aspect; // Width / Height;
        float m_far;  // Far and near are positive and m_far > m_near > 0.
        float m_near;
        bool  m_active;
    };

    class PointLightComponent
    {
    public:
        PointLightComponent(
            float* pColor,
            float affectRadius) :
            m_radius(affectRadius)
        {
            memcpy(m_color, pColor, 3 * sizeof(float));
        }

        void Seralize(YAML::Emitter& emitter);
        void Deseralize(YAML::Node& node);

        float m_color[3];
        float m_radius;
    };

    class ImageBasedLightingComponent
    {
    public:
        ImageBasedLightingComponent() :
            m_diffuseIrradianceGUID(0),
            m_envBrdfGUID(0),
            m_prefilterEnvGUID(0)
        {}

        void Seralize(YAML::Emitter& emitter) {}
        void Deseralize(YAML::Node& node) {}

        uint64_t    m_diffuseIrradianceGUID;
        std::string m_diffuseIrradianceTextureAssetNamePath;

        uint64_t    m_prefilterEnvGUID;
        std::string m_prefilterEnvAssetNamePath;

        uint64_t    m_envBrdfGUID;
        std::string m_envBrdfAssetNamePath;
    };
}
