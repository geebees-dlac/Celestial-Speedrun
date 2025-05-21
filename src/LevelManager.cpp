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

    // Initialize bodyTypeMap (same as your existing)
    m_bodyTypeMap["none"] = phys::bodyType::none;
    m_bodyTypeMap["platform"] = phys::bodyType::platform;
    m_bodyTypeMap["conveyorBelt"] = phys::bodyType::conveyorBelt;
    m_bodyTypeMap["moving"] = phys::bodyType::moving;
    m_bodyTypeMap["interactible"] = phys::bodyType::interactible; // If you use it
    m_bodyTypeMap["falling"] = phys::bodyType::falling;
    m_bodyTypeMap["vanishing"] = phys::bodyType::vanishing;
    m_bodyTypeMap["spring"] = phys::bodyType::spring;
    m_bodyTypeMap["trap"] = phys::bodyType::trap;
    m_bodyTypeMap["solid"] = phys::bodyType::solid;
    m_bodyTypeMap["goal"] = phys::bodyType::goal;

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

// Core request function
bool LevelManager::requestLoadLevel(int levelNumber, LevelData& outLevelData, LoadRequestType type) {
    if (m_transitionState != TransitionState::NONE) {
        std::cerr << "LevelManager Warning: Cannot request load, transition in progress." << std::endl;
        return false;
    }

    if (levelNumber <= 0 || (m_maxLevels > 0 && levelNumber > m_maxLevels && type != LoadRequestType::RESPAWN)) {
         // Allow respawn to try and load current level even if it's the last one technically
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
    if (!hasNextLevel() && m_currentLevelNumber != 0) { // m_currentLevelNumber = 0 means no level loaded yet.
        std::cout << "LevelManager: No next level." << std::endl;
        return false;
    }
    int target = (m_currentLevelNumber == 0) ? 1 : m_currentLevelNumber + 1;
    return requestLoadLevel(target, outLevelData, LoadRequestType::NEXT_LEVEL);
}

bool LevelManager::requestRespawnCurrentLevel(LevelData& outLevelData) {
    if (m_currentLevelNumber <= 0) {
        std::cerr << "LevelManager Error: Cannot respawn, no current level loaded." << std::endl;
        return false; // No level to respawn into
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
                        m_loadingSprite.setTexture(m_loadingTexture, true); // true to reset texture rect
                        // Center sprite (example, adjust as needed)
                        sf::FloatRect bounds = m_loadingSprite.getLocalBounds();
                        m_loadingSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                        // Position will be set in draw based on window size
                        m_loadingScreenReady = true;
                        std::cout << "LevelManager: Loaded image " << imageToLoadPath << std::endl;
                    } else {
                        std::cerr << "LevelManager Error: Failed to load loading image: " << imageToLoadPath << std::endl;
                        m_loadingScreenReady = false;
                    }
                } else {
                    std::cout << "LevelManager: No specific loading image set for this load type." << std::endl;
                    m_loadingScreenReady = false; // No image if path is empty
                }
                 // --- PERFORM ACTUAL LOAD ---
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
                m_loadingScreenReady = false; // Clear readiness for next time
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
    bool showLoadingScreenArt = (m_transitionState == TransitionState::LOADING || // During the brief "LOADING" state
                                (m_transitionState == TransitionState::FADING_OUT && m_transitionClock.getElapsedTime().asSeconds() >= m_fadeDuration) || // At full fade out
                                (m_transitionState == TransitionState::FADING_IN && m_transitionClock.getElapsedTime().asSeconds() < m_fadeDuration)); // During fade in

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
        outLevelData.levelNumber = m_targetLevelNumber; // Ensure the level data reflects the loaded level
        std::cout << "LevelManager (internal): Successfully parsed data from " << filename << std::endl;
    } else {
        std::cerr << "LevelManager (internal): Failed to parse level data structure from " << filename << std::endl;
    }
    return parseSuccess;
}


// --- JSON Parsing Methods (Unchanged from your version, ensure they match if you had local modifications) ---
// readJsonFile, freeJsonDocument, stringToBodyType, parseLevelData
// Ensure your stringToBodyType maps "trap" correctly.
// Ensure parseLevelData clears outLevelData.platforms and outLevelData.movingPlatformDetails.
// Copying your provided parseLevelData, readJsonFile, freeJsonDocument, stringToBodyType from previous working versions

phys::bodyType LevelManager::stringToBodyType(const std::string& typeStr) {
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

    rapidjson::Document* d = new rapidjson::Document(); // latest
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
        outLevelData.levelNumber = d["levelNumber"].GetInt(); // Store what's in JSON, then overwrite later
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
        outLevelData.playerStartPosition = {100.f, 100.f}; // Default
    }

    if (d.HasMember("backgroundColor") && d["backgroundColor"].IsObject()) {
        const auto& bc = d["backgroundColor"];
        sf::Uint8 r = 20, g = 20, b = 40, a = 255; 
        if (bc.HasMember("r") && bc["r"].IsUint()) r = static_cast<sf::Uint8>(bc["r"].GetUint());
        if (bc.HasMember("g") && bc["g"].IsUint()) g = static_cast<sf::Uint8>(bc["g"].GetUint());
        if (bc.HasMember("b") && bc["b"].IsUint()) b = static_cast<sf::Uint8>(bc["b"].GetUint());
        if (bc.HasMember("a") && bc["a"].IsUint()) a = static_cast<sf::Uint8>(bc["a"].GetUint()); // Optional alpha
        outLevelData.backgroundColor = sf::Color(r, g, b, a);
    } else {
        std::cerr << "LevelManager Parse Warning: 'backgroundColor' missing. Using default." << std::endl;
         outLevelData.backgroundColor = sf::Color(20, 20, 40); // Default
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
                id = static_cast<unsigned int>(outLevelData.platforms.size() + 1000); // Basic auto-ID to avoid 0
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
            } else { std::cerr << "Platform ID " << id << " missing position.\n"; }


            // Using width/height from JSON, not size.x/size.y for consistency if you change your JSON
            float width = 32.f, height = 32.f; 
            if (platJson.HasMember("size") && platJson["size"].IsObject()) {
                const auto& s = platJson["size"];
                if (s.HasMember("width") && s["width"].IsNumber()) width = s["width"].GetFloat();
                if (s.HasMember("height") && s["height"].IsNumber()) height = s["height"].GetFloat();
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
                mpi.id = id; //check platform ID or im screwed
                mpi.startPosition = pos; 
                if (mov.HasMember("startPosition") && mov["startPosition"].IsObject()) {
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
        }
    } else {
        std::cerr << "LevelManager Error: 'platforms' array missing or not an array in JSON." << std::endl;
        return false; 
    }
    return true;
}