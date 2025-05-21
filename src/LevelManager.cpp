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
      m_nextLevelLoadingScreenPath("..//assets/images/loading.jpeg"),
      m_respawnLoadingScreenPath("../assets/images/respawn.png") {

    m_bodyTypeMap["none"] = phys::bodyType::none;
    m_bodyTypeMap["platform"] = phys::bodyType::platform;
    m_bodyTypeMap["conveyorBelt"] = phys::bodyType::conveyorBelt;
    m_bodyTypeMap["moving"] = phys::bodyType::moving;
    m_bodyTypeMap["interactible"] = phys::bodyType::interactible; // Ensure this is mapped
    m_bodyTypeMap["falling"] = phys::bodyType::falling;
    m_bodyTypeMap["vanishing"] = phys::bodyType::vanishing;
    m_bodyTypeMap["spring"] = phys::bodyType::spring;
    m_bodyTypeMap["trap"] = phys::bodyType::trap;
    m_bodyTypeMap["solid"] = phys::bodyType::solid;
    m_bodyTypeMap["goal"] = phys::bodyType::goal;

    m_fadeOverlay.setFillColor(sf::Color(0, 0, 0, 0));
}

LevelManager::~LevelManager() {}

// ... (setGeneralLoadingScreenImage, etc. remain the same) ...
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
    outLevelData.interactiblePlatformDetails.clear(); // <-- CLEAR NEW VECTOR

    if (d.HasMember("levelName") && d["levelName"].IsString()) {
        outLevelData.levelName = d["levelName"].GetString();
    } else {
        outLevelData.levelName = "Unnamed Level";
         std::cerr << "LevelManager Parse Warning: 'levelName' missing or not string." << std::endl;
    }

    if (d.HasMember("levelNumber") && d["levelNumber"].IsInt()) {
           int jsonLevelNum = d["levelNumber"].GetInt();
           if (jsonLevelNum != m_targetLevelNumber && m_targetLevelNumber !=0 ) {
               std::cerr << "LevelManager Parse Warning: JSON levelNumber (" << jsonLevelNum
                         << ") mismatches target load (" << m_targetLevelNumber << ")." << std::endl;
           }
        outLevelData.levelNumber = d["levelNumber"].GetInt();
    } else {
        std::cerr << "LevelManager Parse Warning: 'levelNumber' missing or not an int." << std::endl;
    }

    if (d.HasMember("playerStart") && d["playerStart"].IsObject()) {
        const auto& ps = d["playerStart"];
        if (ps.HasMember("x") && ps["x"].IsNumber()) outLevelData.playerStartPosition.x = ps["x"].GetFloat();
        else std::cerr << "LevelManager Parse Warning: playerStart.x missing/not number." << std::endl;
        if (ps.HasMember("y") && ps["y"].IsNumber()) outLevelData.playerStartPosition.y = ps["y"].GetFloat();
        else std::cerr << "LevelManager Parse Warning: playerStart.y missing/not number." << std::endl;
    } else {
        std::cerr << "LevelManager Parse Warning: 'playerStart' missing or not object." << std::endl;
        outLevelData.playerStartPosition = {100.f, 100.f};
    }

    if (d.HasMember("backgroundColor") && d["backgroundColor"].IsObject()) {
        const auto& bc = d["backgroundColor"];
        sf::Uint8 r = 20, g_json = 20, b_json = 40, a_json = 255; // Renamed to avoid conflict in this scope
        if (bc.HasMember("r") && bc["r"].IsUint()) r = static_cast<sf::Uint8>(bc["r"].GetUint());
        if (bc.HasMember("g") && bc["g"].IsUint()) g_json = static_cast<sf::Uint8>(bc["g"].GetUint());
        if (bc.HasMember("b") && bc["b"].IsUint()) b_json = static_cast<sf::Uint8>(bc["b"].GetUint());
        if (bc.HasMember("a") && bc["a"].IsUint()) a_json = static_cast<sf::Uint8>(bc["a"].GetUint());
        outLevelData.backgroundColor = sf::Color(r, g_json, b_json, a_json);
    } else {
        std::cerr << "LevelManager Parse Warning: 'backgroundColor' missing. Using default." << std::endl;
         outLevelData.backgroundColor = sf::Color(20, 20, 40);
    }

    if (d.HasMember("platforms") && d["platforms"].IsArray()) {
        const auto& platformsArray = d["platforms"];
        outLevelData.platforms.reserve(platformsArray.Size());

        for (rapidjson::SizeType i = 0; i < platformsArray.Size(); ++i) {
            const auto& platJson = platformsArray[i];
            if (!platJson.IsObject()) {
                std::cerr << "LevelManager Parse Warning: Platform entry not an object." << std::endl;
                continue;
            }

            unsigned int id = 0;
            if (platJson.HasMember("id") && platJson["id"].IsUint()) {
                id = platJson["id"].GetUint();
            } else {
                std::cerr << "LevelManager Parse Warning: Platform missing 'id' or not Uint. Auto-assigning." << std::endl;
                id = static_cast<unsigned int>(outLevelData.platforms.size() + outLevelData.movingPlatformDetails.size() + outLevelData.interactiblePlatformDetails.size() + 1000);
            }

            std::string typeStr = "solid";
            if (platJson.HasMember("type") && platJson["type"].IsString()) {
                typeStr = platJson["type"].GetString();
            }
            phys::bodyType type = stringToBodyType(typeStr); // Use the member function

            sf::Vector2f pos = {0.f, 0.f};
            if (platJson.HasMember("position") && platJson["position"].IsObject()) {
                const auto& p_json = platJson["position"]; // Renamed
                if (p_json.HasMember("x") && p_json["x"].IsNumber()) pos.x = p_json["x"].GetFloat();
                if (p_json.HasMember("y") && p_json["y"].IsNumber()) pos.y = p_json["y"].GetFloat();
            } else { std::cerr << "Platform ID " << id << " missing position.\n"; }

            float width = 32.f, height = 32.f;
            if (platJson.HasMember("size") && platJson["size"].IsObject()) {
                const auto& s_json = platJson["size"]; // Renamed
                if (s_json.HasMember("width") && s_json["width"].IsNumber()) width = s_json["width"].GetFloat();
                if (s_json.HasMember("height") && s_json["height"].IsNumber()) height = s_json["height"].GetFloat();
            }  else { std::cerr << "Platform ID " << id << " missing size.\n"; }

            sf::Vector2f surfaceVel = {0.f, 0.f};
            if (platJson.HasMember("surfaceVelocity") && platJson["surfaceVelocity"].IsObject()) {
                const auto& sv = platJson["surfaceVelocity"];
                if (sv.HasMember("x") && sv["x"].IsNumber()) surfaceVel.x = sv["x"].GetFloat();
                if (sv.HasMember("y") && sv["y"].IsNumber()) surfaceVel.y = sv["y"].GetFloat();
            }

            bool initiallyFalling = false;
            if (platJson.HasMember("initiallyFalling") && platJson["initiallyFalling"].IsBool()) {
               initiallyFalling = platJson["initiallyFalling"].GetBool();
            }

            outLevelData.platforms.emplace_back(id, pos, width, height, type, initiallyFalling, surfaceVel);

            if (type == phys::bodyType::moving && platJson.HasMember("movement") && platJson["movement"].IsObject()) {
                const auto& mov = platJson["movement"];
                LevelData::MovingPlatformInfo mpi;
                mpi.id = id;
                mpi.startPosition = pos;
                if (mov.HasMember("startPosition") && mov["startPosition"].IsObject()) { // Allow override of start pos for movement calc
                    const auto& msp = mov["startPosition"];
                    if (msp.HasMember("x") && msp["x"].IsNumber()) mpi.startPosition.x = msp["x"].GetFloat();
                    if (msp.HasMember("y") && msp["y"].IsNumber()) mpi.startPosition.y = msp["y"].GetFloat();
                }
                if (mov.HasMember("axis") && mov["axis"].IsString()) {
                    std::string axisStr = mov["axis"].GetString();
                    if (!axisStr.empty()) mpi.axis = std::tolower(axisStr[0]);
                    else std::cerr << "Warning: Moving platform ID " << id << " has empty axis." << std::endl;
                }
                if (mov.HasMember("distance") && mov["distance"].IsNumber()) {
                    mpi.distance = mov["distance"].GetFloat();
                }
                if (mov.HasMember("cycleDuration") && mov["cycleDuration"].IsNumber()) {
                    mpi.cycleDuration = mov["cycleDuration"].GetFloat();
                     if (mpi.cycleDuration <= 0.f) {
                        std::cerr << "Warning: Non-positive cycleDuration for moving platform " << id << ". Defaulting." << std::endl;
                        mpi.cycleDuration = 4.f;
                     }
                }
                 if (mov.HasMember("initialDirection") && mov["initialDirection"].IsInt()) {
                    mpi.initialDirection = mov["initialDirection"].GetInt();
                    if(mpi.initialDirection != 1 && mpi.initialDirection != -1) {
                        std::cerr << "Warning: Invalid initialDirection for moving platform " << id << ". Defaulting." << std::endl;
                        mpi.initialDirection = 1;
                    }
                }
                outLevelData.movingPlatformDetails.push_back(mpi);
            }
            // PARSE INTERACTIBLE DETAILS
            else if (type == phys::bodyType::interactible && platJson.HasMember("interaction") && platJson["interaction"].IsObject()) {
                const auto& inter = platJson["interaction"];
                LevelData::InteractiblePlatformInfo ipi;
                ipi.id = id;

                if (inter.HasMember("type") && inter["type"].IsString()) {
                    ipi.interactionType = inter["type"].GetString();
                    // For now, only "changeSelf" is meaningful, but good to parse
                }
                if (inter.HasMember("targetBodyType") && inter["targetBodyType"].IsString()) {
                    ipi.targetBodyTypeStr = inter["targetBodyType"].GetString();
                } else {
                    std::cerr << "LevelManager Parse Error: Interactible platform ID " << id << " 'interaction' block missing 'targetBodyType' string." << std::endl;
                    // This interactible won't work correctly. Could skip adding it or add with default.
                    ipi.targetBodyTypeStr = "solid"; // Default to solid if missing
                }

                if (inter.HasMember("targetTileColor") && inter["targetTileColor"].IsObject()) {
                    const auto& tc = inter["targetTileColor"];
                    sf::Uint8 r_tc = 0, g_tc = 0, b_tc = 0, a_tc = 255;
                    if (tc.HasMember("r") && tc["r"].IsUint()) r_tc = static_cast<sf::Uint8>(tc["r"].GetUint());
                    if (tc.HasMember("g") && tc["g"].IsUint()) g_tc = static_cast<sf::Uint8>(tc["g"].GetUint());
                    if (tc.HasMember("b") && tc["b"].IsUint()) b_tc = static_cast<sf::Uint8>(tc["b"].GetUint());
                    if (tc.HasMember("a") && tc["a"].IsUint()) a_tc = static_cast<sf::Uint8>(tc["a"].GetUint());
                    ipi.targetTileColor = sf::Color(r_tc, g_tc, b_tc, a_tc);
                    ipi.hasTargetTileColor = true;
                }

                if (inter.HasMember("oneTime") && inter["oneTime"].IsBool()) {
                    ipi.oneTime = inter["oneTime"].GetBool();
                }
                if (inter.HasMember("cooldown") && inter["cooldown"].IsNumber()) {
                    ipi.cooldown = inter["cooldown"].GetFloat();
                }
                outLevelData.interactiblePlatformDetails.push_back(ipi);
            }
        }
    } else {
        std::cerr << "LevelManager Error: 'platforms' array missing or not an array in JSON." << std::endl;
        return false;
    }
    return true;
}