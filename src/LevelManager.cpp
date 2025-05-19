#include "LevelManager.hpp"
#include <cstdio> 
#include <stdexcept> 

rapidjson::Document* readFile(const std::string& filePath){
    FILE* fp = fopen(filePath.c_str(), "r");
    if (!fp) {
        std::cerr << "Error: Could not open level file: " << filePath << std::endl;
        return nullptr;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document* d = new rapidjson::Document();
    d->ParseStream(is);

    fclose(fp);

    if (d->HasParseError()){
        std::cerr << "Error parsing JSON from file: " << filePath << " - Error code " << d->GetParseError() << " at offset " << d->GetErrorOffset() << std::endl;
        delete d;
        return nullptr; 
    }

    if (d->HasMember("lvlcode") && (*d)["lvlcode"].IsInt()) {
        std::cout << "LOADING Level " << (*d)["lvlcode"].GetInt() << std::endl;
    } else {
        std::cout << "LOADING Level (lvlcode not found or not an int)" << std::endl;
    }


    return d;
}

bool loadPlatformsFromDocument(const rapidjson::Document& doc,
                               std::vector<phys::PlatformBody>& bodies,
                               unsigned int& current_id_counter,
                               sf::Vector2f& playerStartPos,
                               size_t& outMovingPlatformID,
                               sf::Vector2f& outMovingPlatformStartPos) {
    bodies.clear(); 

    // Load Player Start Position
    if (doc.HasMember("playerStart") && doc["playerStart"].IsObject()) {
        const auto& pStart = doc["playerStart"];
        if (pStart.HasMember("x") && pStart["x"].IsNumber() &&
            pStart.HasMember("y") && pStart["y"].IsNumber()) {
            playerStartPos.x = pStart["x"].GetFloat();
            playerStartPos.y = pStart["y"].GetFloat();
        } else {
            std::cerr << "Warning: 'playerStart' object is missing x/y or they are not numbers. Using default." << std::endl;
            playerStartPos = sf::Vector2f(0.f, 0.f); // Default position
        }
    } else {
        std::cerr << "Warning: 'playerStart' object not found. Using default player start position." << std::endl;
    }


    if (!doc.HasMember("platforms") || !doc["platforms"].IsArray()) {
        std::cerr << "Error: Level JSON does not contain 'platforms' array." << std::endl;
        return false;
    } else {
        const rapidjson::Value& platformsArray = doc["platforms"];
        for (rapidjson::SizeType i = 0; i < platformsArray.Size(); ++i) {
            const rapidjson::Value& platJson = platformsArray[i];

            if (!platJson.IsObject()) {
                std::cerr << "Warning: Platform entry " << i << " is not an object. Skipping." << std::endl;
                continue;
            }

            // Check for required fields
            if (!platJson.HasMember("x") || !platJson["x"].IsNumber() ||
                !platJson.HasMember("y") || !platJson["y"].IsNumber() ||
                !platJson.HasMember("width") || !platJson["width"].IsNumber() ||
                !platJson.HasMember("height") || !platJson["height"].IsNumber() ||
                !platJson.HasMember("type") || !platJson["type"].IsString()) {
                std::cerr << "Warning: Platform entry " << i << " is missing required fields (x,y,width,height,type) or they have wrong types. Skipping." << std::endl;
                continue;
            }

            float x = platJson["x"].GetFloat();
            float y = platJson["y"].GetFloat();
            sf::Vector2f pos(x, y);
            float width = platJson["width"].GetFloat();
            float height = platJson["height"].GetFloat();
            std::string typeStr = platJson["type"].GetString();
            phys::bodyType type;
            try {
                type = stringToBodyType(typeStr);
            } catch (const std::runtime_error& e) {
                std::cerr << "Warning: Platform entry " << i << " has invalid type '" << typeStr << "'. Skipping. Error: " << e.what() << std::endl;
                continue;
            }

            sf::Vector2f surfaceVel(0.f, 0.f);
            if (platJson.HasMember("surfaceVelocityX") && platJson["surfaceVelocityX"].IsNumber()) {
                surfaceVel.x = platJson["surfaceVelocityX"].GetFloat();
            }
            if (platJson.HasMember("surfaceVelocityY") && platJson["surfaceVelocityY"].IsNumber()) {
                surfaceVel.y = platJson["surfaceVelocityY"].GetFloat();
            }

            unsigned int id_to_use = current_id_counter;
            if (platJson.HasMember("id") && platJson["id"].IsUint()) {
                id_to_use = platJson["id"].GetUint();
                if (id_to_use >= current_id_counter) {
                    current_id_counter = id_to_use + 1;
                }
            } else {
                current_id_counter++;
            }

            bodies.emplace_back(id_to_use, pos, width, height, type, false, surfaceVel);

            if (type == phys::bodyType::moving) {
                if (outMovingPlatformID == (size_t)-1) { 
                    outMovingPlatformID = id_to_use;
                    outMovingPlatformStartPos = pos;
                } else {
                    std::cout << "Warning: Multiple 'moving' platforms found. Using the first one for main animation." << std::endl;
                }
            }
        }
    }
    return true;
}