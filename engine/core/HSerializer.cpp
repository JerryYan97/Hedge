#include "HSerializer.h"
#include "../scene/HScene.h"
#include "HEvent.h"
#include "HEntity.h"
#include "Utils.h"
#include <fstream>
#include <vector>
#include <map>

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
        
        sceneYmlEmitter << YAML::BeginMap;
        sceneYmlEmitter << YAML::Key << "Scene Entities";
        sceneYmlEmitter << YAML::Value;
        sceneYmlEmitter << YAML::BeginMap;

        std::unordered_map<uint32_t, HEntity*>& entitiesHashTbl = scene.GetEntityHashTable();
        for (auto& itr : entitiesHashTbl)
        {
            HEntity* pEntity = itr.second;
            RegisterClassInfo info = m_dict[pEntity->GetClassNameHash()];
            info.pfnSerialize(sceneYmlEmitter, pEntity);
        }

        sceneYmlEmitter << YAML::EndMap;
        sceneYmlEmitter << YAML::EndMap;
    }

    // ================================================================================================================
    void HSerializer::DeserializeYamlToScene(
        const std::string& yamlNamePath,
        HScene& scene,
        HEventManager& eventManager)
    {
        YAML::Node config = YAML::LoadFile(yamlNamePath.c_str());

        bool isMap = config["Scene Entities"].IsMap();

        std::map<std::string, YAML::Node> entities = config["Scene Entities"].as<std::map<std::string, YAML::Node>>();

        for (auto itr : entities)
        {
            const std::string& entityName = itr.first;
            std::string entityType = itr.second["Type"].as<std::string>();

            RegisterClassInfo info = m_dict[crc32(entityType.c_str())];
            HEntity* pEntity = info.pfnDeserialize(itr.second["Components"], entityName);
            // scene.SpawnEntity(pEntity, eventManager);
        }
    }
}