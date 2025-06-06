#include "LevelManager.hpp"
#include "SpriteManager.hpp"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <set>

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
      m_generalLoadingScreenPath("../assets/images/Loading-screen.png"),
      m_nextLevelLoadingScreenPath("../assets/images/Loading-screen.jpeg"),
      m_respawnLoadingScreenPath("../assets/images/respawn.png"), 
      m_loadingJsonDoc(nullptr), 
      m_textureLoadIndex(0){

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

LevelManager::~LevelManager() {freeJsonDocument(m_loadingJsonDoc);} //close mid load
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

void LevelManager::update(float dt, sf::RenderWindow& window, bool isFullscreen) {
    if (m_transitionState == TransitionState::NONE) {
        return;
    }
    float elapsedTime = m_transitionClock.getElapsedTime().asSeconds();
    sf::Color color = m_fadeOverlay.getFillColor();
    switch (m_transitionState) {
        case TransitionState::FADING_OUT: {
            float alpha = std::min(255.f, (elapsedTime / m_fadeDuration) * 255.f);
            color.a = static_cast<uint8_t>(alpha); //chakto lahi nman diay ni :(
            m_fadeOverlay.setFillColor(color);
            if (elapsedTime >= m_fadeDuration) {
                color.a = 255;
                m_fadeOverlay.setFillColor(color);
                
                // Prepare the loading screen graphic itself  
                m_transitionState = TransitionState::LOADING; // instant moving load state
                m_transitionClock.restart();
                std::string imageToLoadPath;
                switch (m_currentLoadType) {
                    case LoadRequestType::NEXT_LEVEL: imageToLoadPath = m_nextLevelLoadingScreenPath; break;
                    case LoadRequestType::RESPAWN:    imageToLoadPath = m_respawnLoadingScreenPath;   break;
                    default:                          imageToLoadPath = m_generalLoadingScreenPath; break;
                }
                if (!imageToLoadPath.empty()) {
                    if (m_loadingTexture.loadFromFile(imageToLoadPath)) {
                        m_loadingTexture.setSmooth(true);
                        m_loadingSprite.emplace(m_loadingTexture);
                        // resize to fit window

                        if (imageToLoadPath == m_generalLoadingScreenPath)
                            m_loadingSprite->setTextureRect(sf::IntRect({0,0}, {1920,1080}));
                        // resize to fit window
                        float scaleX = 1.0f;
                        float scaleY = 1.0f;
                        if (isFullscreen){
                            // fullscreen logic
                            scaleX = 800.0f / m_loadingSprite->getTextureRect().size.x;
                            scaleY = 600.0f / m_loadingSprite->getTextureRect().size.y;
                        }
                        else {
                            // windowed logic
                            scaleX = static_cast<float>(window.getSize().x) / m_loadingSprite->getTextureRect().size.x;
                            scaleY = static_cast<float>(window.getSize().y) / m_loadingSprite->getTextureRect().size.y;
                        }
                        m_loadingSprite->setScale({scaleX, scaleY});
                        m_loadingScreenReady = true;
                        std::cout << "LevelManager: Loaded loading screen image " << imageToLoadPath << std::endl;
                    } else {
                        std::cerr << "LevelManager Error: Failed to load loading image: " << imageToLoadPath << std::endl;
                        m_loadingScreenReady = false;
                    }
                } else {
                    m_loadingScreenReady = false;
                }

                // Prepare the actual level for async loading ---
                if (!m_levelDataToFill) {
                     std::cerr << "LevelManager Critical Error: m_levelDataToFill is null when starting load." << std::endl;
                     m_transitionState = TransitionState::NONE;
                     break;
                }
                
                // clean
                freeJsonDocument(m_loadingJsonDoc);
                m_loadingJsonDoc = nullptr;
                m_texturePathsToLoad.clear();

                std::string filename = m_levelBasePath + "level" + std::to_string(m_targetLevelNumber) + ".json";
                m_loadingJsonDoc = readJsonFile(filename);

                if (!m_loadingJsonDoc) {
                    std::cerr << "LevelManager Error: Failed to read/parse " << filename << ". Aborting load." << std::endl;
                    m_transitionState = TransitionState::NONE; m_levelDataToFill = nullptr;
                    break;
                }

                // Parse everything except the texture files which increases effificneyc
                if (!parseLevelData(*m_loadingJsonDoc, *m_levelDataToFill) || !prepareAsynchronousLoad(*m_loadingJsonDoc, *m_levelDataToFill)) {
                    std::cerr << "LevelManager Error: Failed to prepare level " << m_targetLevelNumber << " for loading." << std::endl;
                    freeJsonDocument(m_loadingJsonDoc); m_loadingJsonDoc = nullptr;
                    m_transitionState = TransitionState::NONE; m_levelDataToFill = nullptr;
                    break;
                }

                m_textureLoadIndex = 0; //load textured in indecise
                std::cout << "LevelManager: Ready to load " << m_texturePathsToLoad.size() << " textures asynchronously." << std::endl;
            }
            break;
        }
        case TransitionState::LOADING:
            if (m_levelDataToFill) {
                processLoadingTick(); //new loading process
            } else {
                std::cerr << "LevelManager Critical Error: m_levelDataToFill is null during LOADING state." << std::endl;
                m_transitionState = TransitionState::NONE;
            }
            break;
        case TransitionState::FADING_IN: {
            float alpha = std::max(0.f, 255.f - (elapsedTime / m_fadeDuration) * 255.f);
            color.a = static_cast<uint8_t>(alpha);
            m_fadeOverlay.setFillColor(color);
            if (elapsedTime >= m_fadeDuration) {
                color.a = 0;
                m_fadeOverlay.setFillColor(color);
                m_transitionState = TransitionState::NONE;
                m_levelDataToFill = nullptr;
                m_loadingScreenReady = false;
                m_texturePathsToLoad.clear();
                freeJsonDocument(m_loadingJsonDoc); //final checks
                m_loadingJsonDoc = nullptr;
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
        //m_loadingSprite->setPosition({window.getSize().x / 2.f, window.getSize().y / 2.f});
        window.draw(*m_loadingSprite);
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
    int jsonLevelNum = 0;

    outLevelData.platforms.clear();
    outLevelData.movingPlatformDetails.clear();
    outLevelData.interactiblePlatformDetails.clear();
    outLevelData.portalPlatformDetails.clear();  
    if (d.HasMember("levelName") && d["levelName"].IsString()) {
        outLevelData.levelName = d["levelName"].GetString();
    } else {
        outLevelData.levelName = "Unnamed Level";
         std::cerr << "LevelManager Parse Warning: 'levelName' missing or not string." << std::endl;
    }

    if (d.HasMember("levelNumber") && d["levelNumber"].IsInt()) {
           jsonLevelNum = d["levelNumber"].GetInt();
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
        uint8_t r = 20, g_json = 20, b_json = 40, a_json = 255; 
        if (bc.HasMember("r") && bc["r"].IsUint()) r = bc["r"].GetUint();
        if (bc.HasMember("g") && bc["g"].IsUint()) g_json = bc["g"].GetUint();
        if (bc.HasMember("b") && bc["b"].IsUint()) b_json = bc["b"].GetUint();
        if (bc.HasMember("a") && bc["a"].IsUint()) a_json = bc["a"].GetUint();
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
            float width = 50.f, height = 50.f; // Default values if not specified
            if (platJson.HasMember("size") && platJson["size"].IsObject()) {
                const auto& sizeJson = platJson["size"];
                width = sizeJson.HasMember("width") ? sizeJson["width"].GetFloat() : width;
                height = sizeJson.HasMember("height") ? sizeJson["height"].GetFloat() : height;
            } else { std::cerr << "Platform ID " << id << " missing size, using defaults.\n"; } // Added warning for missing size

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

            // Parse Body Type
            phys::bodyType type = phys::bodyType::solid; 
            if (platJson.HasMember("type") && platJson["type"].IsString()) {
                type = stringToBodyType(platJson["type"].GetString());
            }

            // Parse individual texture (path)
            std::string texturePath = DEFAULT_TEXTURE_FILEPATH;
            if (platJson.HasMember("texture") && platJson["texture"].IsString()){
                texturePath = platJson["texture"].GetString();
            }

            // Create Base Platform
            outLevelData.platforms.emplace_back(
                id, pos, width, height, type, initiallyFalling, surfaceVel, texturePath //checkpoint
            );
            
            phys::PlatformBody& justAddedBody = outLevelData.platforms.back();

            // Handle Special Types
            if (type == phys::bodyType::portal) {
                LevelData::PortalPlatformInfo ppi;
                ppi.id = id;

                // Parse PortalID (Required)
                if (platJson.HasMember("portalID") && platJson["portalID"].IsUint()) {
                    ppi.portalID = platJson["portalID"].GetUint();
                } else {
                    std::cerr << "Portal missing portalID, ID: " << id << "\n";
                    ppi.portalID = 0;
                    continue;
                }

                // Parse Teleport Offset (Optional)
                if (platJson.HasMember("teleportOffset") && platJson["teleportOffset"].IsObject()) {
                    const auto& offsetJson = platJson["teleportOffset"];
                    ppi.offset.x = offsetJson.HasMember("x") && offsetJson["x"].IsNumber() ? offsetJson["x"].GetFloat() : 10.f;
                    ppi.offset.y = offsetJson.HasMember("y") && offsetJson["y"].IsNumber() ? offsetJson["y"].GetFloat() : 0.f;
                } else {
                     ppi.offset = {10.f, 0.f}; 
                }
                justAddedBody.setPortalID(ppi.portalID);
                justAddedBody.setTeleportOffset(ppi.offset);

                outLevelData.portalPlatformDetails.push_back(ppi);
            }

            else if (type == phys::bodyType::moving && platJson.HasMember("movement") && platJson["movement"].IsObject()) {
                const auto& mov = platJson["movement"];
                LevelData::MovingPlatformInfo mpi;
                mpi.id = id;
                mpi.startPosition = pos; // Use the platform's general 'pos' as default start, override if specified in 'movement'
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
                        std::cerr << "Warning: Non-positive cycleDuration for moving platform " << id << ". Defaulting to 4s." << std::endl;
                        mpi.cycleDuration = 4.f;
                     }
                }
                 if (mov.HasMember("initialDirection") && mov["initialDirection"].IsInt()) {
                    mpi.initialDirection = mov["initialDirection"].GetInt();
                    if(mpi.initialDirection != 1 && mpi.initialDirection != -1) {
                        std::cerr << "Warning: Invalid initialDirection for moving platform " << id << ". Defaulting to 1." << std::endl;
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
                }
                if (inter.HasMember("targetBodyType") && inter["targetBodyType"].IsString()) {
                    ipi.targetBodyTypeStr = inter["targetBodyType"].GetString();
                } else {
                    std::cerr << "LevelManager Parse Error: Interactible platform ID " << id << " 'interaction' block missing 'targetBodyType' string. Defaulting to 'solid'." << std::endl;
                    ipi.targetBodyTypeStr = "solid"; 
                }

                if (inter.HasMember("targetTileColor") && inter["targetTileColor"].IsObject()) {
                    const auto& tc = inter["targetTileColor"];
                    uint8_t r_tc = 0, g_tc = 0, b_tc = 0, a_tc = 255;
                    if (tc.HasMember("r") && tc["r"].IsUint()) r_tc = tc["r"].GetUint();
                    if (tc.HasMember("g") && tc["g"].IsUint()) g_tc = tc["g"].GetUint();
                    if (tc.HasMember("b") && tc["b"].IsUint()) b_tc = tc["b"].GetUint();
                    if (tc.HasMember("a") && tc["a"].IsUint()) a_tc = tc["a"].GetUint();
                    ipi.targetTileColor = sf::Color(r_tc, g_tc, b_tc, a_tc);
                    ipi.hasTargetTileColor = true;
                }

                if (inter.HasMember("oneTime") && inter["oneTime"].IsBool()) {
                    ipi.oneTime = inter["oneTime"].GetBool();
                }
                if (inter.HasMember("cooldown") && inter["cooldown"].IsNumber()) {
                    ipi.cooldown = inter["cooldown"].GetFloat();
                }
                 if (inter.HasMember("linkedID") && inter["linkedID"].IsUint()) { // Added linkedID parsing
                    ipi.linkedID = inter["linkedID"].GetUint();
                }
                outLevelData.interactiblePlatformDetails.push_back(ipi); // Ensure this is added for interactibles
            
            }
        } 

    } else {
        std::cerr << "LevelManager Error: Missing platforms array\n";
        return false;
    }

    return true; //remove unecessary debug
}

bool LevelManager::prepareAsynchronousLoad(const rapidjson::Document& d, LevelData& outLevelData) {
    // clear ram
    outLevelData.TexturesList.clear();
    outLevelData.TexturesDimensions.clear();
    m_texturePathsToLoad.clear();


    std::set<std::string> uniquePaths;
    uniquePaths.insert(DEFAULT_TEXTURE_FILEPATH);


    if (d.HasMember("platforms") && d["platforms"].IsArray()) {
        const auto& platformsArray = d["platforms"];
        for (rapidjson::SizeType i = 0; i < platformsArray.Size(); ++i) {
            const auto& platJson = platformsArray[i];
            if (!platJson.IsObject()) continue;

            // Collect list
            if (platJson.HasMember("texture") && platJson["texture"].IsString()) {
                std::string path = platJson["texture"].GetString();
                
                if (path.find(TEXTURE_DIRECTORY) == std::string::npos)
                    path = TEXTURE_DIRECTORY + path;
                uniquePaths.insert(path);
            }

            // We scan and parse at the same time
            if (platJson.HasMember("dimensions") && platJson["dimensions"].IsObject()) {
                const auto& dimensions = platJson["dimensions"];
                unsigned int id = 0;
                if (platJson.HasMember("id") && platJson["id"].IsUint()) {
                    id = platJson["id"].GetUint();
                }

                if (id != 0 && dimensions.HasMember("top-left-x") && dimensions["top-left-x"].IsInt()
                    && dimensions.HasMember("top-left-y") && dimensions["top-left-y"].IsInt()
                    && dimensions.HasMember("bottom-right-x") && dimensions["bottom-right-x"].IsInt()
                    && dimensions.HasMember("bottom-right-y") && dimensions["bottom-right-y"].IsInt())
                    {
                        outLevelData.TexturesDimensions.emplace(id,
                            sf::IntRect({dimensions["top-left-x"].GetInt(), dimensions["top-left-y"].GetInt()},
                                {dimensions["bottom-right-x"].GetInt() - dimensions["top-left-x"].GetInt(),
                                dimensions["bottom-right-y"].GetInt() - dimensions["top-left-y"].GetInt()}));
                    }
            }
        }
    }

    // Also check for the special level background texture (wgat?)
    if (d.HasMember("backgroundTexture") && d["backgroundTexture"].IsString()) {
        std::string path = d["backgroundTexture"].GetString();
        if (path.find(IMAGE_DIRECTORY) == std::string::npos) {
            path = IMAGE_DIRECTORY + path;
        }
        uniquePaths.insert(path);
    }

    // Now copy the unique paths into our texture loading list
    m_texturePathsToLoad.assign(uniquePaths.begin(), uniquePaths.end());

    return true;
}

void LevelManager::processLoadingTick() {
    // check if finished loading texture
    if (m_textureLoadIndex >= m_texturePathsToLoad.size()) {
        std::cout << "LevelManager: Asynchronous loading complete." << std::endl;
        m_currentLevelNumber = m_targetLevelNumber;

        // fade in now
        m_transitionState = TransitionState::FADING_IN;
        m_transitionClock.restart();
        return; // exit
    }

    // keep load texture path
    const std::string& path_to_load = m_texturePathsToLoad[m_textureLoadIndex];

    // handle background case
    std::string key_to_use = path_to_load;
    if (m_loadingJsonDoc->HasMember("backgroundTexture") && (*m_loadingJsonDoc)["backgroundTexture"].IsString()) {
        std::string bg_path_json = (*m_loadingJsonDoc)["backgroundTexture"].GetString();
        std::string bg_path_full = (bg_path_json.find(IMAGE_DIRECTORY) == std::string::npos) ? (IMAGE_DIRECTORY + bg_path_json) : bg_path_json;

        if (path_to_load == bg_path_full) {
            key_to_use = LEVEL_BG_ID; // Use the special identifier for the background
        }
    }

    sf::Texture newTexture;
    std::cout << "Loading texture: " << path_to_load << "..." << std::endl;
    if (!newTexture.loadFromFile(path_to_load)) {
        std::cerr << "LevelManager Error: Failed to load texture '" << path_to_load << "'. Using default." << std::endl;
        newTexture.loadFromFile(DEFAULT_TEXTURE_FILEPATH); // Use fallback
        key_to_use = DEFAULT_TEXTURE_FILEPATH; // enuse matches
    }

    // store in lvl data
    if (m_levelDataToFill && m_levelDataToFill->TexturesList.find(key_to_use) == m_levelDataToFill->TexturesList.end()) {
        m_levelDataToFill->TexturesList.emplace(key_to_use, std::move(newTexture));
    }

    // advance to next texture
    m_textureLoadIndex++;
}