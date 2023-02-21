#include "HEntity.h"
#include "../scene/HScene.h"
#include "../render/HRenderManager.h"
#include "HComponent.h"
#include <sstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Hedge
{
    // ================================================================================================================
    HEntity::HEntity()
        : m_pScene(nullptr),
          m_entityHandle(0)
    {}

    // ================================================================================================================
    HEntity::~HEntity()
    {}

    // ================================================================================================================
    void HEntity::CreateInSceneInternal(
        HScene*  pScene,
        uint32_t handle)
    {
        m_pScene         = pScene;
        m_entityHandle   = handle;
    }

    // ================================================================================================================
    template<typename Type, typename... Args>
    void HEntity::AddComponent(
        Args &&...args)
    {
        m_pScene->EntityAddComponent<Type>(m_entityHandle, std::forward<Args>(args)...);
    }

    // ================================================================================================================
    template<typename T>
    T& HEntity::GetComponent()
    {
        return m_pScene->EntityGetComponent<T>(m_entityHandle);
    }

    // ================================================================================================================
    HCubeEntity::~HCubeEntity()
    {
        StaticMeshComponent& meshComponent = GetComponent<StaticMeshComponent>();
        delete meshComponent.m_pIdx;
        delete meshComponent.m_pVert;
    }

    // ================================================================================================================
    void HCubeEntity::OnDefineEntity()
    {
        //
        float pos[3] = {0.f, 0.f, 1.f};
        float rot[4] = {0.f, 0.f, 0.f, 0.f};
        float scale[3] = {1.f, 1.f, 1.f};

        AddComponent<TransformComponent>(pos, rot, scale);

        // pos1, pos2, pos3, uv0, uv1, normal0, normal1, normal2
        // pos1, pos2, pos3, col1, col2, col3
        float* verts = new float[] {
                -0.75f, -0.75f, 0.f, 1.f, 0.f, 0.f, // v0 - Top Left
                0.75f, -0.75f, 0.f, 0.f, 1.f, 0.f, // v1 - Top Right
                0.75f, 0.75f, 0.f, 0.f, 0.f, 1.f, // v2 - Bottom Right
                -0.75f, 0.75f, 0.f, 1.f, 1.f, 0.f // v3 - Bottom Left
        };

        // CCW
        // v0 - v1 - v2; v2 - v3 - v0;
        uint32_t* vertIdx = new uint32_t[] {
            0, 1, 2, 2, 3, 0
        };

        std::string cubeObj("o Cube \n\
v 1.000000 1.000000 -1.000000 \n\
v 1.000000 -1.000000 -1.000000 \n\
v 1.000000 1.000000 1.000000 \n\
v 1.000000 -1.000000 1.000000 \n\
v -1.000000 1.000000 -1.000000 \n\
v -1.000000 -1.000000 -1.000000 \n\
v -1.000000 1.000000 1.000000 \n\
v -1.000000 -1.000000 1.000000 \n\
vt 0.875000 0.500000 \n\
vt 0.625000 0.750000 \n\
vt 0.625000 0.500000 \n\
vt 0.375000 1.000000 \n\
vt 0.375000 0.750000 \n\
vt 0.625000 0.000000 \n\
vt 0.375000 0.250000 \n\
vt 0.375000 0.000000 \n\
vt 0.375000 0.500000 \n\
vt 0.125000 0.750000 \n\
vt 0.125000 0.500000 \n\
vt 0.625000 0.250000 \n\
vt 0.875000 0.750000 \n\
vt 0.625000 1.000000 \n\
vn 0.0000 1.0000 0.0000 \n\
vn 0.0000 0.0000 1.0000 \n\
vn -1.0000 0.0000 0.0000 \n\
vn 0.0000 -1.0000 0.0000 \n\
vn 1.0000 0.0000 0.0000 \n\
vn 0.0000 0.0000 -1.0000 \n\
s off \n\
f 5/1/1 3/2/1 1/3/1 \n\
f 3/2/2 8/4/2 4/5/2 \n\
f 7/6/3 6/7/3 8/8/3 \n\
f 2/9/4 8/10/4 6/11/4 \n\
f 1/3/5 4/5/5 2/9/5 \n\
f 5/12/6 2/9/6 6/7/6 \n\
f 5/1/1 7/13/1 3/2/1 \n\
f 3/2/2 7/14/2 8/4/2 \n\
f 7/6/3 5/12/3 6/7/3 \n\
f 2/9/4 4/5/4 8/10/4 \n\
f 1/3/5 3/2/5 4/5/5 \n\
f 5/12/6 1/3/6 2/9/6");

        std::string cubeObj2("o Cube");

        std::string s0("Initial string");

        std::vector<tinyobj::material_t> materials;
        std::map<std::string, uint32_t> textures;
        tinyobj::attrib_t inattrib;
        std::vector<tinyobj::shape_t> inshapes;
        std::string warn;
        std::string err;
        std::istringstream istr(cubeObj);

        tinyobj::LoadObj(&inattrib, &inshapes, &materials, &warn, &err, &istr);

        AddComponent<StaticMeshComponent>(vertIdx,
                                          verts,
                                          6);
    }

    // ================================================================================================================
    HCameraEntity::~HCameraEntity()
    {}

    // ================================================================================================================
    void HCameraEntity::OnDefineEntity()
    {
        float pos[3] = { 0.f, 0.f, -1.f };
        float rot[4] = { 0.f, 0.f, 0.f, 0.f };
        float scale[3] = { 1.f, 1.f, 1.f };

        AddComponent<TransformComponent>(pos, rot, scale);

        float view[3] = { 0.f, 0.f, 1.f };
        float up[3] = { 0.f, 1.f, 0.f };
        float fov = 60.f * M_PI / 180.f;
        float aspect = 960.f / 680.f;

        AddComponent<CameraComponent>(view, up, fov, aspect, 0.1f, 10.f);
    }
}