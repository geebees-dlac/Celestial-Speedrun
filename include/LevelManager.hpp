#ifndef LEVEL_MANAGER_HPP
#define LEVEL_MANAGER_HPP

#include "rapidjson/document.h"
#include "PlatformBody.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"

#include <string>
#include <vector>
#include <map>
#include "PhysicsTypes.hpp"

namespace phys {}

struct LevelData {
    // level handler of the intial rules
    std::string levelName;
    int levelNumber = 0;
    sf::Vector2f playerStartPosition = {100.f, 100.f};
    sf::Color backgroundColor = sf::Color(20, 20, 40);
    std::vector<phys::PlatformBody> platforms;

    //moving platform rules
    struct MovingPlatformInfo {
        unsigned int id;
        sf::Vector2f startPosition;
        char axis = 'x';
        float distance = 0.f;
        float cycleDuration = 4.f;
        int initialDirection = 1;
    };
    std::vector<MovingPlatformInfo> movingPlatformDetails;

    //interactible platform rules
    struct InteractiblePlatformInfo {
        unsigned int id;
        std::string interactionType = "changeSelf"; 
        std::string targetBodyTypeStr;             
        sf::Color targetTileColor = sf::Color::Transparent; 
        bool hasTargetTileColor = false;
        bool oneTime = false;
        float cooldown = 0.0f;
        unsigned int linkedID = 0;
    };
    std::vector<InteractiblePlatformInfo> interactiblePlatformDetails; 

    //portal rules
    struct PortalPlatformInfo {
    unsigned int id; 
    unsigned int portalID; 
    sf::Vector2f offset{10.f, 0.f}; 
};
    std::vector<PortalPlatformInfo> portalPlatformDetails;

    // Sprites and textures
    std::map<std::string, sf::Texture> TexturesList;
};

class LevelManager {
public:
    enum class TransitionState {
        NONE,
        FADING_OUT,
        LOADING,
        FADING_IN
    };

    enum class LoadRequestType {
        GENERAL,
        NEXT_LEVEL,
        RESPAWN
    };

    LevelManager();
    ~LevelManager();

    void setLevelBasePath(const std::string& path) { m_levelBasePath = path; }
    void setGeneralLoadingScreenImage(const std::string& imagePath);
    void setNextLevelLoadingScreenImage(const std::string& imagePath);
    void setRespawnLoadingScreenImage(const std::string& imagePath);
    void setTransitionProperties(float fadeDuration = 1.0f);

    bool requestLoadLevel(int levelNumber, LevelData& outLevelData, LoadRequestType type = LoadRequestType::GENERAL);
    bool requestLoadSpecificLevel(int levelNumber, LevelData& outLevelData);
    bool requestLoadNextLevel(LevelData& outLevelData);
    bool requestRespawnCurrentLevel(LevelData& outLevelData);

    void update(float dt, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);

    bool isTransitioning() const;
    TransitionState getCurrentTransitionState() const { return m_transitionState; }

    int getCurrentLevelNumber() const { return m_currentLevelNumber; }
    void setCurrentLevelNumber(int number) { m_currentLevelNumber = number; }

    bool hasNextLevel() const;
    void setMaxLevels(int max) { m_maxLevels = max; }

    // Utility to convert string to bodyType - MADE PUBLIC
    phys::bodyType stringToBodyType(const std::string& typeStr) const;


private:
    bool performActualLoad(int levelNumber, LevelData& outLevelData);
    bool loadLevelDataFromFile(const std::string& filename, LevelData& outLevelData);
    bool loadLevelDataFromJson(const rapidjson::Document& doc, LevelData& outLevelData);
    
    rapidjson::Document* readJsonFile(const std::string& filepath);
    void freeJsonDocument(rapidjson::Document* doc);
    // phys::bodyType stringToBodyType(const std::string& typeStr); // Moved to public
    bool parseLevelData(const rapidjson::Document& doc, LevelData& outLevelData);
    bool parseLevelTextures(const rapidjson::Document& doc, LevelData& outLevelData);

    int m_currentLevelNumber;
    int m_targetLevelNumber;
    LevelData* m_levelDataToFill;

    int m_maxLevels;
    std::string m_levelBasePath;
    std::map<std::string, phys::bodyType> m_bodyTypeMap;

    TransitionState m_transitionState;
    LoadRequestType m_currentLoadType;
    sf::Clock m_transitionClock;
    float m_fadeDuration;

    sf::Texture m_loadingTexture;
    std::optional <sf::Sprite> m_loadingSprite;
    bool m_loadingScreenReady;

    std::string m_generalLoadingScreenPath;
    std::string m_nextLevelLoadingScreenPath;
    std::string m_respawnLoadingScreenPath;

    sf::RectangleShape m_fadeOverlay;
};

#endif // LEVEL_MANAGER_HPP