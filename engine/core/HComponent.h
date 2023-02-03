#pragma once
#include <cstdint>

namespace Hedge
{
    struct TransformComponent
    {
        TransformComponent(
            float* pPos, 
            float* pRot, 
            float* pScale)
        {
            m_pos[0]   = pPos[0];   m_pos[1]   = pPos[1];   m_pos[2]   = pPos[2];
            m_rotQ[0]  = pRot[0];   m_rotQ[1]  = pRot[1];   m_rotQ[2]  = pRot[2];   m_rotQ[3] = pRot[3];
            m_scale[0] = pScale[0]; m_scale[1] = pScale[1]; m_scale[2] = pScale[2];
        }
        float m_pos[3];
        float m_rotQ[4];
        float m_scale[3];
    };

    struct StaticMeshComponent
    {
        StaticMeshComponent(
            uint32_t* pIdx,
            float*    pVert,
            uint32_t  vertCnt)
            : m_pIdx(pIdx),
              m_pVert(pVert),
              m_vertCnt(vertCnt)
        {}

        ~StaticMeshComponent()
        {
            delete m_pIdx;
            delete m_pVert;
        }

        uint32_t* m_pIdx  = nullptr;
        float*    m_pVert = nullptr;
        uint32_t  m_vertCnt;
    };
}
