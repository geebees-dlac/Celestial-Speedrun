#ifndef LEVEL_MANAGER_HPP
#define LEVEL_MANAGER_HPP

#include "rapidjson/document.h"
#include "PlatformBody.hpp" // Assuming phys::PlatformBody is here
#include "SFML/System/Vector2.hpp"
#include "SFML/Graphics/Color.hpp" // For background color
#include <string>
#include <vector>
#include <map>
#include "PhysicsTypes.hpp" 
#include "PlatformBody.hpp"

namespace phys {
}

struct LevelData {
    std::string levelName;
    int levelNumber = 0;
    sf::Vector2f playerStartPosition = {100.f, 100.f}; 
    sf::Color backgroundColor = sf::Color(20, 20, 40);   
    std::vector<phys::PlatformBody> platforms;

    // For moving platforms sa platformy body ra dhway ni
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
    LevelManager();

    bool loadLevel(int levelNumber, LevelData& outLevelData);
    
    bool loadLevelFromFile(const std::string& filename, LevelData& outLevelData);

    int getCurrentLevelNumber() const { return m_currentLevelNumber; }
    void setCurrentLevelNumber(int number) { m_currentLevelNumber = number; }
    
    bool hasNextLevel() const;
    bool loadNextLevel(LevelData& outLevelData); 
    
    void setMaxLevels(int max) { m_maxLevels = max; }


private:
    rapidjson::Document* readJsonFile(const std::string& filepath);
    void freeJsonDocument(rapidjson::Document* doc);
    phys::bodyType stringToBodyType(const std::string& typeStr);
    bool parseLevelData(const rapidjson::Document& doc, LevelData& outLevelData);

    int m_currentLevelNumber;
    int m_maxLevels; 
    std::string m_levelBasePath;
    std::map<std::string, phys::bodyType> m_bodyTypeMap;
};

#endif 