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
    std::string levelName;
    int levelNumber = 0;
    sf::Vector2f playerStartPosition = {100.f, 100.f};
    sf::Color backgroundColor = sf::Color(20, 20, 40);
    std::vector<phys::PlatformBody> platforms;

    struct MovingPlatformInfo {
        unsigned int id;
        sf::Vector2f startPosition;
        char axis = 'x';
        float distance = 0.f;
        float cycleDuration = 4.f;
        int initialDirection = 1;
    };
    std::vector<MovingPlatformInfo> movingPlatformDetails;
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


private:
    bool performActualLoad(int levelNumber, LevelData& outLevelData); 
    bool loadLevelDataFromFile(const std::string& filename, LevelData& outLevelData);


    rapidjson::Document* readJsonFile(const std::string& filepath);
    void freeJsonDocument(rapidjson::Document* doc);
    phys::bodyType stringToBodyType(const std::string& typeStr);
    bool parseLevelData(const rapidjson::Document& doc, LevelData& outLevelData);

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
    sf::Sprite m_loadingSprite;
    bool m_loadingScreenReady;

    std::string m_generalLoadingScreenPath;
    std::string m_nextLevelLoadingScreenPath;
    std::string m_respawnLoadingScreenPath;
    

    sf::RectangleShape m_fadeOverlay;
};

#endif // LEVEL_MANAGER_HPP