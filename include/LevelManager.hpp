#ifndef LVLMNGR_HPP
#define LVLMNGR_HPP

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "SFML/Graphics.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>
#include <filesystem> // when needed for levels, i think but now its not needed
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "PlatformBody.hpp" 
#include "PhysicsTypes.hpp"
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/experimental.json");
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/level1.json");
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/level2.json");
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/level3.json");
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/level4.json");
rapidjson::Document* readFile(const std::string& filePath = "../assets/levels/level5.json");

int savetest(rapidjson::Document* d);
void freeData(rapidjson::Document* d);

bool loadPlatformsFromDocument(const rapidjson::Document& doc,
                               std::vector<phys::PlatformBody>& bodies,
                               unsigned int& current_id_counter,
                               sf::Vector2f& playerStartPos, 
                               size_t& outMovingPlatformID, 
                               sf::Vector2f& outMovingPlatformStartPos); 

phys::bodyType stringToBodyType(const std::string& typeStr);
#endif