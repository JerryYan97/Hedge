#pragma once
#include <cstdint>

namespace Hedge
{
    // x -- pitch, y -- head, z -- roll; m_rot[pitch, head, roll].
    struct TransformComponent
    {
        TransformComponent(
            float* pPos, 
            float* pRot, 
            float* pScale)
        {
            memcpy(m_pos, pPos, 3 * sizeof(float));
            memcpy(m_rot, pRot, 3 * sizeof(float));
            memcpy(m_scale, pScale, 3 * sizeof(float));
        }
        float m_pos[3];
        float m_rot[3];
        float m_scale[3];
    };

    struct StaticMeshComponent
    {
        StaticMeshComponent(
            uint32_t* pIdx,
            float*    pVert,
            uint32_t  vertCnt,
            uint32_t  vertBufBytes)
            : m_pIdx(pIdx),
              m_pVert(pVert),
              m_vertCnt(vertCnt),
              m_vertBufBytes(vertBufBytes)
        {}

        ~StaticMeshComponent()
        {
            delete m_pIdx;
            delete m_pVert;
        }

        uint32_t* m_pIdx  = nullptr;
        float*    m_pVert = nullptr;
        uint32_t  m_vertBufBytes;
        uint32_t  m_vertCnt;
    };

    struct CameraComponent
    {
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

        float m_view[3];
        float m_up[3];
        float m_fov;
        float m_aspect; // Width / Height;
        float m_far;  // Far and near are positive and m_far > m_near > 0.
        float m_near;
        bool  m_active;
    };
}
