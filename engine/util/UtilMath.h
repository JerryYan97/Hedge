#pragma once
#include <cstdint>

namespace Hedge
{
    template<typename T>
    inline void MatrixMul4x4(T mat1[16], T mat2[16], T* resMat);

    template<typename T>
    inline T Norm(T* vec, uint32_t dim);

    template<typename T>
    inline bool NormalizeVec(T* vec, uint32_t dim);

    template<typename T>
    inline void CrossProductVec3(T* vec1, T* vec2, T* resVec);

    template<typename T>
    inline T DotProduct(T* vec1, T* vec2, uint32_t dim);

    // Generate 4x4 matrices
    // Realtime rendering -- P67
    inline void GenViewMat(float* pView, float* pPos, float* pUp, float* pResMat);

    // Realtime rendering -- P99. Far are near are posive, which correspond to f' and n'. And far > near.
    inline void GenPerspectiveProjMat(float near, float far, float fov, float aspect, float* pResMat);

    // Realtime rendering -- P70, P65. E = R (roll -- z) * R (pitch -- x) * R (head -- y)
    inline void GenModelMat(float* pPos, float roll, float pitch, float head, float* pScale, float* pResMat);

    inline void GenModelMat(float* pPos, float* pQuat, float* pScale) {};


}