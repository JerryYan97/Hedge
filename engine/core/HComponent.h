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

    // NOTE: Static is not responsible for vertex or index delete.
    class StaticMeshComponent
    {
    public:
        StaticMeshComponent(
            uint32_t* pIdx,
            float*    pVert,
            uint32_t  idxCnt,
            uint32_t  vertBufBytes,
            std::string meshAssetPathName,
            bool      preBuiltMesh)
            : m_pIdx(pIdx),
              m_pVert(pVert),
              m_idxCnt(idxCnt),
              m_vertBufBytes(vertBufBytes),
              m_preBuiltMesh(preBuiltMesh),
              m_meshAssetPathName(meshAssetPathName)
        {}

        ~StaticMeshComponent()
        {}

        void Seralize(YAML::Emitter& emitter);
        void Deseralize(YAML::Node& node);

        uint32_t* m_pIdx  = nullptr;
        float*    m_pVert = nullptr;
        uint32_t  m_vertBufBytes;
        uint32_t  m_idxCnt;
        bool      m_preBuiltMesh;
        std::string m_meshAssetPathName;
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
            float* pColor)
        {
            memcpy(m_color, pColor, 3 * sizeof(float));
        }

        void Seralize(YAML::Emitter& emitter) {};
        void Deseralize(YAML::Node& node) {};

        float m_color[3];
    };
}
