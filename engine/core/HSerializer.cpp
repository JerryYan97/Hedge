#include "HSerializer.h"
#include "../scene/HScene.h"
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
    void HSerializer::RegisterAClass(
        uint32_t nameHash,
        RegisterClassInfo regInfo)
    {
        m_dict.insert({ nameHash, regInfo });
    }

    // ================================================================================================================
    void HSerializer::SerializeScene(
        std::string& yamlNamePath, 
        HScene& scene)
    {
        std::ofstream sceneYmlFileHandle(yamlNamePath.c_str());
        YAML::Emitter sceneYmlEmitter(sceneYmlFileHandle);
        
        std::unordered_map<uint32_t, HEntity*>& entitiesHashTbl = scene.GetEntityHashTable();
        for (auto& itr : entitiesHashTbl)
        {
            
        }
    }

    // ================================================================================================================
    void HSerializer::DeserializeYamlToScene(std::string& yamlNamePath, HScene& scene)
    {}
}