#include "LevelManager.hpp"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"
#include <cstdio>
#include <iostream>
#include <algorithm>

// Constructor
LevelManager::LevelManager()
    : m_currentLevelNumber(0),
      m_targetLevelNumber(0),
      m_levelDataToFill(nullptr),
      m_maxLevels(0),
      m_levelBasePath("../assets/levels/"),
      m_transitionState(TransitionState::NONE),
      m_currentLoadType(LoadRequestType::GENERAL),
      m_fadeDuration(1.0f),
      m_loadingScreenReady(false),
      m_generalLoadingScreenPath("../assets/images/menuload.png"),
      m_nextLevelLoadingScreenPath("../assets/images/loading.jpeg"),
      m_respawnLoadingScreenPath("../assets/images/respawn.png") {

    m_bodyTypeMap["none"] = phys::bodyType::none;
    m_bodyTypeMap["platform"] = phys::bodyType::platform;
    m_bodyTypeMap["conveyorBelt"] = phys::bodyType::conveyorBelt;
    m_bodyTypeMap["moving"] = phys::bodyType::moving;
    m_bodyTypeMap["interactible"] = phys::bodyType::interactible;
    m_bodyTypeMap["falling"] = phys::bodyType::falling;
    m_bodyTypeMap["vanishing"] = phys::bodyType::vanishing;
    m_bodyTypeMap["spring"] = phys::bodyType::spring;
    m_bodyTypeMap["trap"] = phys::bodyType::trap;
    m_bodyTypeMap["solid"] = phys::bodyType::solid;
    m_bodyTypeMap["goal"] = phys::bodyType::goal;
    m_bodyTypeMap["portal"] = phys::bodyType::portal;

    m_fadeOverlay.setFillColor(sf::Color(0, 0, 0, 0));
}

LevelManager::~LevelManager() {}
void LevelManager::setGeneralLoadingScreenImage(const std::string& imagePath) {
    m_generalLoadingScreenPath = imagePath;
}
void LevelManager::setNextLevelLoadingScreenImage(const std::string& imagePath) {
    m_nextLevelLoadingScreenPath = imagePath;
}
void LevelManager::setRespawnLoadingScreenImage(const std::string& imagePath) {
    m_respawnLoadingScreenPath = imagePath;
}

void LevelManager::setTransitionProperties(float fadeDuration) {
    m_fadeDuration = std::max(0.1f, fadeDuration);
}

bool LevelManager::requestLoadLevel(int levelNumber, LevelData& outLevelData, LoadRequestType type) {
    if (m_transitionState != TransitionState::NONE) {
        std::cerr << "LevelManager Warning: Cannot request load, transition in progress." << std::endl;
        return false;
    }
    if (levelNumber <= 0 || (m_maxLevels > 0 && levelNumber > m_maxLevels && type != LoadRequestType::RESPAWN)) {
        if (!(type == LoadRequestType::RESPAWN && levelNumber == m_currentLevelNumber && m_currentLevelNumber > 0)){
             std::cerr << "LevelManager Error: Requested level " << levelNumber << " invalid." << std::endl;
             return false;
        }
    }
    m_targetLevelNumber = levelNumber;
    m_levelDataToFill = &outLevelData;
    m_currentLoadType = type;
    m_transitionState = TransitionState::FADING_OUT;
    m_transitionClock.restart();
    m_loadingScreenReady = false;
    std::cout << "LevelManager: FADE_OUT for level " << m_targetLevelNumber << " (Type: " << static_cast<int>(type) << ")" << std::endl;
    return true;
}
bool LevelManager::requestLoadSpecificLevel(int levelNumber, LevelData& outLevelData) {
    return requestLoadLevel(levelNumber, outLevelData, LoadRequestType::GENERAL);
}
bool LevelManager::requestLoadNextLevel(LevelData& outLevelData) {
    if (!hasNextLevel() && m_currentLevelNumber != 0) {
        std::cout << "LevelManager: No next level." << std::endl;
        return false;
    }
    int target = (m_currentLevelNumber == 0) ? 1 : m_currentLevelNumber + 1;
    return requestLoadLevel(target, outLevelData, LoadRequestType::NEXT_LEVEL);
}
bool LevelManager::requestRespawnCurrentLevel(LevelData& outLevelData) {
    if (m_currentLevelNumber <= 0) {
        std::cerr << "LevelManager Error: Cannot respawn, no current level loaded." << std::endl;
        return false;
    }
    return requestLoadLevel(m_currentLevelNumber, outLevelData, LoadRequestType::RESPAWN);
}

void LevelManager::update(float dt, sf::RenderWindow& window) {
    if (m_transitionState == TransitionState::NONE) {
        return;
    }
    float elapsedTime = m_transitionClock.getElapsedTime().asSeconds();
    sf::Color color = m_fadeOverlay.getFillColor();
    switch (m_transitionState) {
        case TransitionState::FADING_OUT: {
            float alpha = std::min(255.f, (elapsedTime / m_fadeDuration) * 255.f);
            color.a = static_cast<sf::Uint8>(alpha);
            m_fadeOverlay.setFillColor(color);
            if (elapsedTime >= m_fadeDuration) {
                color.a = 255;
                m_fadeOverlay.setFillColor(color);
                m_transitionState = TransitionState::LOADING;
                m_transitionClock.restart();
                std::string imageToLoadPath;
                switch (m_currentLoadType) {
                    case LoadRequestType::NEXT_LEVEL: imageToLoadPath = m_nextLevelLoadingScreenPath; break;
                    case LoadRequestType::RESPAWN:    imageToLoadPath = m_respawnLoadingScreenPath;   break;
                    case LoadRequestType::GENERAL:
                    default:                          imageToLoadPath = m_generalLoadingScreenPath; break;
                }
                if (!imageToLoadPath.empty()) {
                    if (m_loadingTexture.loadFromFile(imageToLoadPath)) {
                        m_loadingTexture.setSmooth(true);
                        m_loadingSprite.setTexture(m_loadingTexture, true);
                        sf::FloatRect bounds = m_loadingSprite.getLocalBounds();
                        m_loadingSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                        m_loadingScreenReady = true;
                        std::cout << "LevelManager: Loaded image " << imageToLoadPath << std::endl;
                    } else {
                        std::cerr << "LevelManager Error: Failed to load loading image: " << imageToLoadPath << std::endl;
                        m_loadingScreenReady = false;
                    }
                } else {
                    std::cout << "LevelManager: No specific loading image set for this load type." << std::endl;
                    m_loadingScreenReady = false;
                }
                if (m_levelDataToFill) {
                    if (performActualLoad(m_targetLevelNumber, *m_levelDataToFill)) {
                        m_currentLevelNumber = m_targetLevelNumber;
                         std::cout << "LevelManager: Level " << m_targetLevelNumber << " loaded successfully." << std::endl;
                        m_transitionState = TransitionState::FADING_IN;
                        m_transitionClock.restart();
                        std::cout << "LevelManager: Transitioning to FADING_IN." << std::endl;
                    } else {
                        std::cerr << "LevelManager Error: Failed to load level " << m_targetLevelNumber << " data." << std::endl;
                        m_transitionState = TransitionState::NONE;
                        m_levelDataToFill = nullptr;
                    }
                } else {
                     std::cerr << "LevelManager Critical Error: m_levelDataToFill is null during LOADING." << std::endl;
                     m_transitionState = TransitionState::NONE;
                }
            }
            break;
        }
        case TransitionState::LOADING:
            break;
        case TransitionState::FADING_IN: {
            float alpha = std::max(0.f, 255.f - (elapsedTime / m_fadeDuration) * 255.f);
            color.a = static_cast<sf::Uint8>(alpha);
            m_fadeOverlay.setFillColor(color);
            if (elapsedTime >= m_fadeDuration) {
                color.a = 0;
                m_fadeOverlay.setFillColor(color);
                m_transitionState = TransitionState::NONE;
                m_levelDataToFill = nullptr;
                m_loadingScreenReady = false;
                std::cout << "LevelManager: FADING_IN complete. Transition finished." << std::endl;
            }
            break;
        }
        case TransitionState::NONE:
            break;
    }
    m_fadeOverlay.setSize(sf::Vector2f(window.getSize()));
}

void LevelManager::draw(sf::RenderWindow& window) {
    bool showLoadingScreenArt = (m_transitionState == TransitionState::LOADING ||
                                (m_transitionState == TransitionState::FADING_OUT && m_transitionClock.getElapsedTime().asSeconds() >= m_fadeDuration) ||
                                (m_transitionState == TransitionState::FADING_IN && m_transitionClock.getElapsedTime().asSeconds() < m_fadeDuration));
    if (showLoadingScreenArt && m_loadingScreenReady) {
        m_loadingSprite.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f);
        window.draw(m_loadingSprite);
    }
    if (m_fadeOverlay.getFillColor().a > 0) {
        window.draw(m_fadeOverlay);
    }
}

bool LevelManager::isTransitioning() const {
    return m_transitionState != TransitionState::NONE;
}

bool LevelManager::hasNextLevel() const {
    if (m_maxLevels > 0) {
        return m_currentLevelNumber < m_maxLevels;
    }
    return true;
}

bool LevelManager::performActualLoad(int levelNumber, LevelData& outLevelData) {
    std::string filename = m_levelBasePath + "level" + std::to_string(levelNumber) + ".json";
    std::cout << "LevelManager: Performing actual load of: " << filename << std::endl;
    return loadLevelDataFromFile(filename, outLevelData);
}

bool LevelManager::loadLevelDataFromFile(const std::string& filename, LevelData& outLevelData) {
    std::cout << "LevelManager (internal): Reading JSON from: " << filename << std::endl;
    rapidjson::Document* doc = readJsonFile(filename);
    if (!doc) {
        std::cerr << "LevelManager: Failed to read/parse " << filename << std::endl;
        return false;
    }
    bool parseSuccess = parseLevelData(*doc, outLevelData);
    freeJsonDocument(doc);
    if (parseSuccess) {
        outLevelData.levelNumber = m_targetLevelNumber;
        std::cout << "LevelManager (internal): Successfully parsed data from " << filename << std::endl;
    } else {
        std::cerr << "LevelManager (internal): Failed to parse level data structure from " << filename << std::endl;
    }
    return parseSuccess;
}

// MADE PUBLIC and CONST
phys::bodyType LevelManager::stringToBodyType(const std::string& typeStr) const {
    auto it = m_bodyTypeMap.find(typeStr);
    if (it != m_bodyTypeMap.end()) {
        return it->second;
    }
    std::cerr << "LevelManager Warning: Unknown bodyType string: '" << typeStr << "'. Defaulting to 'solid'." << std::endl;
    return phys::bodyType::solid;
}

rapidjson::Document* LevelManager::readJsonFile(const std::string& filepath) {
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp) {
        std::cerr << "LevelManager Error: Could not open JSON file: " << filepath << std::endl;
        return nullptr;
    }
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document* d = new rapidjson::Document();
    d->ParseStream(is);
    fclose(fp);
    if (d->HasParseError()) {
        std::cerr << "LevelManager Error parsing JSON: " << filepath << std::endl;
        std::cerr << "Error (offset " << d->GetErrorOffset() << "): "
                  << rapidjson::GetParseError_En(d->GetParseError()) << std::endl;
        delete d;
        return nullptr;
    }
    return d;
}

void LevelManager::freeJsonDocument(rapidjson::Document* doc) {
    if (doc) {
        delete doc;
    }
}

bool LevelManager::parseLevelData(const rapidjson::Document& d, LevelData& outLevelData) {
    outLevelData.platforms.clear();
    outLevelData.movingPlatformDetails.clear();
    outLevelData.interactiblePlatformDetails.clear();
    outLevelData.portalPlatformDetails.clear();  

    if (d.HasMember("platforms") && d["platforms"].IsArray()) {
        const auto& platformsArray = d["platforms"];
        outLevelData.platforms.reserve(platformsArray.Size());

        for (rapidjson::SizeType i = 0; i < platformsArray.Size(); ++i) {
            const auto& platJson = platformsArray[i];
            if (!platJson.IsObject()) continue;

            // Parse Common Properties
            unsigned int id = 0;
            if (platJson.HasMember("id") && platJson["id"].IsUint()) {
                id = platJson["id"].GetUint();
            } else {
                id = static_cast<unsigned int>(outLevelData.platforms.size() + 1000);
                std::cerr << "Auto-assigned ID: " << id << " to missing ID platform\n";
            }

            // Parse Position
            sf::Vector2f pos{0, 0};
            if (platJson.HasMember("position") && platJson["position"].IsObject()) {
                const auto& posJson = platJson["position"];
                pos.x = posJson.HasMember("x") ? posJson["x"].GetFloat() : 0;
                pos.y = posJson.HasMember("y") ? posJson["y"].GetFloat() : 0;
            }

            // Parse Size 
            float width = 50.f, height = 50.f;
            if (platJson.HasMember("size") && platJson["size"].IsObject()) {
                const auto& sizeJson = platJson["size"];
                width = sizeJson.HasMember("width") ? sizeJson["width"].GetFloat() : width;
                height = sizeJson.HasMember("height") ? sizeJson["height"].GetFloat() : height;
            }

            // Parse Body Type
            phys::bodyType type = phys::bodyType::solid;
            if (platJson.HasMember("type") && platJson["type"].IsString()) {
                type = stringToBodyType(platJson["type"].GetString());
            }

            // Create Base Platform
            outLevelData.platforms.emplace_back(
                id, pos, width, height, type,
                /* initiallyFalling */ false, 
                /* surfaceVel */ sf::Vector2f{0, 0}
            );

            // Handle Special Types
            if (type == phys::bodyType::portal) {
                LevelData::PortalPlatformInfo ppi;
                ppi.id = id;

                // Parse PortalID (Required)
                if (platJson.HasMember("portalID") && platJson["portalID"].IsUint()) {
                    ppi.portalID = platJson["portalID"].GetUint();
                } else {
                    std::cerr << "Portal missing portalID, ID: " << id << "\n";
                    continue;
                }

                // Parse Teleport Offset (Optional)
                if (platJson.HasMember("teleportOffset") && platJson["teleportOffset"].IsObject()) {
                    const auto& offset = platJson["teleportOffset"];
                    ppi.offset.x = offset.HasMember("x") ? offset["x"].GetFloat() : 10.f;
                    ppi.offset.y = offset.HasMember("y") ? offset["y"].GetFloat() : 0.f;
                }

                outLevelData.portalPlatformDetails.push_back(ppi);
            
            }
                else if (type == phys::bodyType::portal) {
                    LevelData::PortalPlatformInfo ppi;
                    ppi.id = id;

                    // Parse portalID (required)
                    if (platJson.HasMember("portalID") && platJson["portalID"].IsUint()) {
                        ppi.portalID = platJson["portalID"].GetUint();
                    } else {
                        std::cerr << "Portal ID " << id << " missing portalID\n";
                        continue;
                    }

                    // Parse offset (optional)
                    if (platJson.HasMember("teleportOffset") && platJson["teleportOffset"].IsObject()) {
                        const auto& offset = platJson["teleportOffset"];
                        if (offset.HasMember("x")) ppi.offset.x = offset["x"].GetFloat();
                        if (offset.HasMember("y")) ppi.offset.y = offset["y"].GetFloat();
                    }

                    outLevelData.portalPlatformDetails.push_back(ppi);
                }
        }
    } else {
        std::cerr << "LevelManager Error: Missing platforms array\n";
        return false;
    }
    return true;
}