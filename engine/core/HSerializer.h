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
}

typedef void (*PFN_CLASSREG)(YAML::Node& node);

namespace Hedge
{
    class HScene;

    class HSerializer
    {
    public:
        HSerializer();
        ~HSerializer();

        void RegisterAClass(uint32_t nameHash, PFN_CLASSREG pFunc);
        void SerializeScene(std::string& yamlNamePath, HScene& scene);
        void DeserializeYamlToScene(std::string& yamlNamePath, HScene& scene);

    protected:


    private:
        std::unordered_map<uint32_t, PFN_CLASSREG> m_dict; // Class name hash -- Class instance reg function.
    };
}