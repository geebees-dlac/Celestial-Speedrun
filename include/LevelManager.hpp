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

rapidjson::Document* readFile();
int savetest(rapidjson::Document* d);
void freeData(rapidjson::Document* d);

#endif