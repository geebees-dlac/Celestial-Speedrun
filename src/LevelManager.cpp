#include "LevelManager.hpp"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h" // For GetParseError_En
#include <iostream>
#include <cstdio> 
#include "PhysicsTypes.hpp"


LevelManager::LevelManager() 
    : m_currentLevelNumber(0),
      m_maxLevels(5),         
      m_levelBasePath("../assets/levels/") { 

    m_bodyTypeMap["none"] = phys::bodyType::none;
    m_bodyTypeMap["platform"] = phys::bodyType::platform;
    m_bodyTypeMap["conveyorBelt"] = phys::bodyType::conveyorBelt;
    m_bodyTypeMap["moving"] = phys::bodyType::moving;
    m_bodyTypeMap["falling"] = phys::bodyType::falling;
    m_bodyTypeMap["vanishing"] = phys::bodyType::vanishing;
    m_bodyTypeMap["spring"] = phys::bodyType::spring;
    m_bodyTypeMap["trap"] = phys::bodyType::trap;
    m_bodyTypeMap["solid"] = phys::bodyType::solid;
    m_bodyTypeMap["goal"] = phys::bodyType::goal;
}

phys::bodyType LevelManager::stringToBodyType(const std::string& typeStr) {
    auto it = m_bodyTypeMap.find(typeStr);
    if (it != m_bodyTypeMap.end()) {
        return it->second;
    }
    std::cerr << "Warning: Unknown bodyType string: '" << typeStr << "'. Defaulting to 'solid'." << std::endl;
    return phys::bodyType::solid; 
}

rapidjson::Document* LevelManager::readJsonFile(const std::string& filepath) {
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Error: Could not open JSON file: " << filepath << std::endl;
        return nullptr;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document* d = new rapidjson::Document();
    d->ParseStream(is);
    fclose(fp);

    if (d->HasParseError()) {
        std::cerr << "Error parsing JSON file: " << filepath << std::endl;
        std::cerr << "Error (offset " << d->GetErrorOffset() << "): "
                  << rapidjson::GetParseError_En(d->GetParseError()) << std::endl;
        delete d;
        return nullptr;
    }
    return d;
}

void LevelManager::freeJsonDocument(rapidjson::Document* doc) {
    delete doc;
}

bool LevelManager::parseLevelData(const rapidjson::Document& d, LevelData& outLevelData) {
    outLevelData.platforms.clear();
    outLevelData.movingPlatformDetails.clear(); 

    if (d.HasMember("levelName") && d["levelName"].IsString()) {
        outLevelData.levelName = d["levelName"].GetString();
    } else {
        outLevelData.levelName = "Unnamed Level";
    }

    if (d.HasMember("levelNumber") && d["levelNumber"].IsInt()) {
        outLevelData.levelNumber = d["levelNumber"].GetInt();
    } else {
        std::cerr << "Warning: 'levelNumber' not found or not an int in JSON." << std::endl;
    }
    
    if (d.HasMember("playerStart") && d["playerStart"].IsObject()) {
        const auto& ps = d["playerStart"];
        if (ps.HasMember("x") && ps["x"].IsNumber() && ps.HasMember("y") && ps["y"].IsNumber()) {
            outLevelData.playerStartPosition.x = ps["x"].GetFloat();
            outLevelData.playerStartPosition.y = ps["y"].GetFloat();
        } else {
             std::cerr << "Warning: 'playerStart' object missing 'x' or 'y' or they are not numbers." << std::endl;
        }
    } else {
        std::cerr << "Warning: 'playerStart' not found or not an object in JSON." << std::endl;
    }

    if (d.HasMember("backgroundColor") && d["backgroundColor"].IsObject()) {
        const auto& bc = d["backgroundColor"];
        uint8_t r = 20, g = 20, b = 40; // UNSURE FIX PLS VERIFY
        if (bc.HasMember("r") && bc["r"].IsUint()) r = bc["r"].GetUint();
        if (bc.HasMember("g") && bc["g"].IsUint()) g = bc["g"].GetUint();
        if (bc.HasMember("b") && bc["b"].IsUint()) b = bc["b"].GetUint();
        outLevelData.backgroundColor = sf::Color(r, g, b);
    }


    if (d.HasMember("platforms") && d["platforms"].IsArray()) {
        const auto& platformsArray = d["platforms"];
        outLevelData.platforms.reserve(platformsArray.Size());

        for (rapidjson::SizeType i = 0; i < platformsArray.Size(); ++i) {
            const auto& platJson = platformsArray[i];
            if (!platJson.IsObject()) continue;

            unsigned int id = 0;
            if (platJson.HasMember("id") && platJson["id"].IsUint()) {
                id = platJson["id"].GetUint();
            } else {
                std::cerr << "Warning: Platform missing 'id' or not Uint. Assigning sequential ID." << std::endl;
                id = static_cast<unsigned int>(outLevelData.platforms.size()); // Or skip
            }

            std::string typeStr = "solid";
            if (platJson.HasMember("type") && platJson["type"].IsString()) {
                typeStr = platJson["type"].GetString();
            }
            phys::bodyType type = stringToBodyType(typeStr);

            sf::Vector2f pos = {0.f, 0.f};
            if (platJson.HasMember("position") && platJson["position"].IsObject()) {
                const auto& p = platJson["position"];
                if (p.HasMember("x") && p["x"].IsNumber()) pos.x = p["x"].GetFloat();
                if (p.HasMember("y") && p["y"].IsNumber()) pos.y = p["y"].GetFloat();
            }

            sf::Vector2f size = {32.f, 32.f}; // Default size
            if (platJson.HasMember("size") && platJson["size"].IsObject()) {
                const auto& s = platJson["size"];
                if (s.HasMember("width") && s["width"].IsNumber()) size.x = s["width"].GetFloat();
                if (s.HasMember("height") && s["height"].IsNumber()) size.y = s["height"].GetFloat();
            }

            sf::Vector2f surfaceVel = {0.f, 0.f};
            if (platJson.HasMember("surfaceVelocity") && platJson["surfaceVelocity"].IsObject()) {
                const auto& sv = platJson["surfaceVelocity"];
                if (sv.HasMember("x") && sv["x"].IsNumber()) surfaceVel.x = sv["x"].GetFloat();
                if (sv.HasMember("y") && sv["y"].IsNumber()) surfaceVel.y = sv["y"].GetFloat();
            }
            
            //made by intellisense might be useful later
            // Handle falling platforms
            // InitiallyFalling is false by default for PlatformBody constructor. Add to JSON if needed.
            // bool initiallyFalling = false; 
            // if (platJson.HasMember("initiallyFalling") && platJson["initiallyFalling"].IsBool()) {
            //    initiallyFalling = platJson["initiallyFalling"].GetBool();
            // }

            outLevelData.platforms.emplace_back(id, pos, size.x, size.y, type, false, surfaceVel);
        
            // --- Handle Moving Platform Specifics ---
            if (type == phys::bodyType::moving && platJson.HasMember("movement") && platJson["movement"].IsObject()) {
                const auto& mov = platJson["movement"];
                LevelData::MovingPlatformInfo mpi;
                mpi.id = id;
                mpi.startPosition = pos; 

                if (mov.HasMember("axis") && mov["axis"].IsString()) {
                    std::string axisStr = mov["axis"].GetString();
                    if (axisStr == "x" || axisStr == "X") mpi.axis = 'x';
                    else if (axisStr == "y" || axisStr == "Y") mpi.axis = 'y';
                    else std::cerr << "Warning: Moving platform ID " << id << " has invalid axis '" << axisStr << "'." << std::endl;
                }
                if (mov.HasMember("distance") && mov["distance"].IsNumber()) {
                    mpi.distance = mov["distance"].GetFloat();
                }
                if (mov.HasMember("cycleDuration") && mov["cycleDuration"].IsNumber()) {
                    mpi.cycleDuration = mov["cycleDuration"].GetFloat();
                     if (mpi.cycleDuration <= 0) mpi.cycleDuration = 4.f; 
                }
                 if (mov.HasMember("initialDirection") && mov["initialDirection"].IsInt()) {
                    mpi.initialDirection = mov["initialDirection"].GetInt();
                    if(mpi.initialDirection == 0) mpi.initialDirection = 1; 
                }
                outLevelData.movingPlatformDetails.push_back(mpi);
            }
        }
    } else {
        std::cerr << "Error: 'platforms' array not found in JSON or not an array." << std::endl;
        return false;
    }
    return true;
}


bool LevelManager::loadLevel(int levelNumber, LevelData& outLevelData) {
    if (levelNumber <= 0 || levelNumber > m_maxLevels) {
        std::cerr << "Error: Level number " << levelNumber << " is out of range (1-" << m_maxLevels << ")." << std::endl;
        return false;
    }
    std::string filename = m_levelBasePath + "level" + std::to_string(levelNumber) + ".json";
    bool success = loadLevelFromFile(filename, outLevelData);
    if(success) {
        m_currentLevelNumber = levelNumber;
        outLevelData.levelNumber = levelNumber;
    }
    return success;
}

bool LevelManager::loadLevelFromFile(const std::string& filename, LevelData& outLevelData) {
    std::cout << "Loading level: " << filename << std::endl;
    rapidjson::Document* doc = readJsonFile(filename);
    if (!doc) {
        return false;
    }

    bool parseSuccess = parseLevelData(*doc, outLevelData);
    freeJsonDocument(doc);

    if (!parseSuccess) {
        std::cerr << "Failed to parse data for level: " << filename << std::endl;
        return false;
    }
    
    std::cout << "Successfully loaded level: " << outLevelData.levelName 
              << " (Number: " << outLevelData.levelNumber 
              << ", Player Start: " << outLevelData.playerStartPosition.x << "," << outLevelData.playerStartPosition.y << ")" << std::endl;
    return true;
}

bool LevelManager::hasNextLevel() const {
    return m_currentLevelNumber < m_maxLevels;
}

bool LevelManager::loadNextLevel(LevelData& outLevelData) {
    if (hasNextLevel()) {
        return loadLevel(m_currentLevelNumber + 1, outLevelData);
    }
    return false; 
}