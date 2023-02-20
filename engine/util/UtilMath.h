#pragma once
#include <cstdint>

namespace Hedge
{
    template<typename T>
    inline void MatrixMul4x4(const T mat1[16], const T mat2[16], T* resMat)
    {
        for (uint32_t row = 0; row < 4; row++)
        {
            for (uint32_t col = 0; col < 4; col++)
            {
                uint32_t idx = 4 * row + col;
                resMat[idx] = mat1[4 * row] * mat2[col] +
                    mat1[4 * row + 1] * mat2[4 + col] +
                    mat1[4 * row + 2] * mat2[8 + col] +
                    mat1[4 * row + 3] * mat2[12 + col];
            }
        }
    }

    template<typename T>
    inline T Norm(T* vec, uint32_t dim)
    {
        T res = 0;
        for (uint32_t i = 0; i < dim; i++)
        {
            res += (vec[i] * vec[i]);
        }
        return sqrt(res);
    }

    template<typename T>
    inline bool NormalizeVec(T* vec, uint32_t dim)
    {
        T l2Norm = Norm(vec, dim);
        if (l2Norm == 0)
        {
            return false;
        }
        else
        {
            for (uint32_t i = 0; i < dim; i++)
            {
                vec[i] = vec[i] / l2Norm;
            }
            return true;
        }
    }

    template<typename T>
    inline void CrossProductVec3(T* vec1, T* vec2, T* resVec)
    {
        resVec[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
        resVec[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
        resVec[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
    }

    template<typename T>
    inline T DotProduct(T* vec1, T* vec2, uint32_t dim)
    {
        T res = 0;
        for (uint32_t i = 0; i < dim; i++)
        {
            res += (vec1[i] * vec2[i]);
        }
        return res;
    }

    // Generate 4x4 matrices
    // Realtime rendering -- P67
    void GenViewMatUpdateUp(float* const pView, float* const pPos, float* pUp, float* pResMat);

    // Realtime rendering -- P99. Far are near are posive, which correspond to f' and n'. And far > near.
    void GenPerspectiveProjMat(float near, float far, float fov, float aspect, float* pResMat);

    // Realtime rendering -- P70, P65. E = R (roll -- z) * R (pitch -- x) * R (head -- y)
    void GenModelMat(float* pPos, float roll, float pitch, float head, float* pScale, float* pResMat);
}