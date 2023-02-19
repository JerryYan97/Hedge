#include "UtilMath.h"
#include <math.h>
#include <cstring>

namespace Hedge
{
    void GenViewMatUpdateUp(
        float* const pView, 
        float* const pPos, 
        float* pUp, 
        float* pResMat)
    {
        float right[3];
        CrossProductVec3(pView, pUp, right);
        NormalizeVec(right, 3);

        CrossProductVec3(right, pView, pUp);
        NormalizeVec(pUp, 3);

        float e03 = -DotProduct(pPos, right,  3);
        float e13 = -DotProduct(pPos, pUp, 3);
        float e23 = -DotProduct(pPos, pView,  3);

        memset(pResMat,     0,      16 * sizeof(float));
        memcpy(pResMat,     right,  sizeof(right));
        memcpy(&pResMat[4], pUp,    3 * sizeof(float));
        memcpy(&pResMat[8], pView,  3 * sizeof(float));
        
        pResMat[3]  = e03;
        pResMat[7]  = e13;
        pResMat[11] = e23;
        pResMat[15] = 1.f;
    }

    void GenPerspectiveProjMat(
        float near, 
        float far, 
        float fov, 
        float aspect,
        float* pResMat)
    {
        memset(pResMat, 0, 16 * sizeof(float));
        
        float c = 1.f / tanf(fov / 2.f);
        pResMat[0] = c / aspect;
        pResMat[5] = c;

        pResMat[10] = -(far + near) / (far - near);
        pResMat[11] = -(2 * far * near) / (far - near);

        pResMat[14] = -1.f;
    }

    void GenModelMat(
        float* pPos, 
        float roll, 
        float pitch, 
        float head, 
        float* pScale, 
        float* pResMat)
    {
        // Concatenation of translation matrix and rotation matrix.
        float tRMat[16] = {};

        tRMat[0] = cosf(roll) * cosf(head) - sinf(roll) * sinf(pitch) * sinf(head);
        tRMat[1] = - sinf(roll) * cosf(pitch);
        tRMat[2] = cosf(roll) * sinf(head) + sinf(roll) * sinf(pitch) * cosf(head);
        tRMat[3] = pPos[0];

        tRMat[4] = sinf(roll) * cosf(head) + cosf(roll) * sinf(pitch) * sinf(head);
        tRMat[5] = cosf(roll) * cosf(pitch);
        tRMat[6] = sinf(roll) * sinf(head) - cosf(roll) * sinf(pitch) * cosf(head);
        tRMat[7] = pPos[1];

        tRMat[8] = - cosf(pitch) * sinf(head);
        tRMat[9] = sinf(pitch);
        tRMat[10] = cosf(pitch) * cosf(head);
        tRMat[11] = pPos[2];

        tRMat[15] = 1.f;

        // Assembly the scale matrix.
        float sMat[16] = {};
        sMat[0] = pScale[0];
        sMat[5] = pScale[1];
        sMat[10] = pScale[2];
        sMat[15] = 1.f;

        // Multiply TR and S matrix together to get the model matrix.
        memset(pResMat, 0, 16 * sizeof(float));
        MatrixMul4x4(tRMat, sMat, pResMat);
    }
}
