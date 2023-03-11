#include "HSerializer.h"
#include <fstream>

namespace Hedge
{
    // ================================================================================================================
    HSerializer::HSerializer()
    {

    }

    // ================================================================================================================
    HSerializer::~HSerializer()
    {

    }

    // ================================================================================================================
    void HSerializer::RegisterAClass(uint32_t nameHash, PFN_CLASSREG pFunc)
    {
        m_dict.insert({ nameHash, pFunc });
    }

    // ================================================================================================================
    void HSerializer::SerializeScene(std::string& yamlNamePath, HScene& scene)
    {
        std::ofstream sceneYmlFileHandle(yamlNamePath.c_str());
        YAML::Emitter sceneYmlEmitter(sceneYmlFileHandle);
        
    }

    // ================================================================================================================
    void HSerializer::DeserializeYamlToScene(std::string& yamlNamePath, HScene& scene)
    {}
}