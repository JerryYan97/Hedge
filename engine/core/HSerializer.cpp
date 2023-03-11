#include "HSerializer.h"

namespace Hedge
{
    HSerializer::HSerializer()
    {

    }

    HSerializer::~HSerializer()
    {

    }

    void HSerializer::RegisterClass(uint32_t nameHash, PFN_CLASSREG pFunc)
    {}

    void HSerializer::SerializeScene(std::string& yamlNamePath, HScene& scene)
    {}

    void HSerializer::DeserializeYamlToScene(std::string& yamlNamePath, HScene& scene)
    {}
}