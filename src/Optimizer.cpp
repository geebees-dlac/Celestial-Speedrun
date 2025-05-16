#include "Optimizer.hpp" // Or your new name like "Easing.hpp"
#include <cmath> // For std::cos, std::pow
#include <vector>
#include <SFML/System/Vector2.hpp> // Add this include for sf::Vector2f
#include "PhysicsTypes.hpp" 
// Using Penner-style functions
float currentTime = 2.5f;   // Elapsed time
float duration = 5.0f;    // Total duration
float startValue = 10.0f;
float endValue = 100.0f;
float change = endValue - startValue;

float easedValue = math::easing::cubicEaseOut(currentTime, startValue, change, duration);

// Using normalized easing functions
float alpha = currentTime / duration; // Normalized time (0 to 1)
float easedAlpha = math::normalized_easing::expoEaseIn(alpha);
float value = math::lerp(startValue, endValue, easedAlpha);
// or
// float value = startValue + (endValue - startValue) * easedAlpha;

sf::Vector2f startPos = {0,0};
sf::Vector2f endPos = {100, 200};
sf::Vector2f currentPos = math::lerp(startPos, endPos, easedAlpha);