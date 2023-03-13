#pragma once
#include <unordered_map>
#include "yaml-cpp/yaml.h"

namespace Hedge
{
    struct HedgeSerializeData
    {
        uint32_t* pIntData;
        uint32_t intDataCnt;

        float* pFloatData;
        uint32_t floatDataCnt;
    };

    class HEntity;
}

typedef void (*PFN_SERIALIZE)(YAML::Emitter& emitter, Hedge::HEntity* pThis);
typedef void (*PFN_DESERIALIZE)(YAML::Node& node, const std::string& name, Hedge::HEntity* pThis);
typedef Hedge::HEntity* (*PFN_NEWENTITY)();

struct RegisterClassInfo
{
    PFN_SERIALIZE pfnSerialize;
    PFN_DESERIALIZE pfnDeserialize;
    PFN_NEWENTITY pfnNewEntity;
};

namespace Hedge
{
    class HScene;
    class HEventManager;

    class HSerializer
    {
    public:
        HSerializer();
        ~HSerializer();

        void RegisterAClass(uint32_t nameHash, RegisterClassInfo regInfo);
        void SerializeScene(std::string& yamlNamePath, HScene& scene);
        void DeserializeYamlToScene(const std::string& yamlNamePath, HScene& scene, HEventManager& eventManager);

    protected:


    private:
        std::unordered_map<uint32_t, RegisterClassInfo> m_dict; // Class name hash -- Class instance reg function.
    };
}