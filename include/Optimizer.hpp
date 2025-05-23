#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <cmath>   // For std::pow, std::cos, std::sin
#include <numbers> // For std::numbers::pi_v (C++20)
//i am not touching anything here cuz idk anything here
// just note none of this is ai but a snippet from a code i found online
// this has gone kaput and i will not be using this until further notice and realized its effects alr
// --- Robert Penner Easing Functions (t, b, c, d pattern) ---
//ty robert penner for da easing functions
// i just copied it like a dumbass
//https://code.markrichards.ninja/sfml/sfml-platformer-in-less-than-1-million-lines-part-2
// t: current time, b: beginning value, c: change in value, d: duration
// Define a PI constant for use within this header, preferring std::numbers
#ifndef PI_FOR_EASING // Use a unique macro name to avoid conflicts
    #if __cplusplus >= 202002L && __has_include(<numbers>)
        #define PI_FOR_EASING std::numbers::pi_v<float>
    #else
        // Fallback if <numbers> not available or not C++20
        // Using acos for a high-precision PI is a common trick.
        #define PI_FOR_EASING static_cast<float>(std::acos(-1.0L))
    #endif
#endif

namespace math { 
namespace easing {

// Linear
inline float linear(float t, float b, float c, float d) {
    if (d == 0.0f) return (t >=d )? b+c : b;
    return c * (t / d) + b;
}

// Exponential
inline float expoEaseIn(float t, float b, float c, float d) {
    if (t == 0.0f) return b;
    if (d == 0.0f) return b+c;
    return c * std::pow(2.0f, 10.0f * (t / d - 1.0f)) + b;
}
inline float expoEaseOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    if (t == d) return b + c;
    return c * (-std::pow(2.0f, -10.0f * t / d) + 1.0f) + b;
}
inline float expoEaseInOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    if (t == 0.0f) return b;
    if (t == d) return b + c;
    t /= d / 2.0f;
    if (t < 1.0f) return c / 2.0f * std::pow(2.0f, 10.0f * (t - 1.0f)) + b;
    return c / 2.0f * (-std::pow(2.0f, -10.0f * --t) + 2.0f) + b;
}

// Cubic
inline float cubicEaseIn(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t /= d;
    return c * t * t * t + b;
}
inline float cubicEaseOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t = t / d - 1.0f;
    return c * (t * t * t + 1.0f) + b;
}
inline float cubicEaseInOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t /= d / 2.0f;
    if (t < 1.0f) return c / 2.0f * t * t * t + b;
    t -= 2.0f;
    return c / 2.0f * (t * t * t + 2.0f) + b;
}

// Quadratic
inline float quadraticEaseIn(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t /= d;
    return c * t * t + b;
}
inline float quadraticEaseOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t /= d;
    return -c * t * (t - 2.0f) + b;
}
inline float quadraticEaseInOut(float t, float b, float c, float d) {
    if (d == 0.0f) return b + c;
    t /= d / 2.0f;
    if (t < 1.0f) return c / 2.0f * t * t + b;
    t--;
    return -c / 2.0f * (t * (t - 2.0f) - 1.0f) + b;
}

// Sine (ADDED)
inline float sineEaseIn(float t, float b, float c, float d) {
    if (d == 0.0f) return (t >=d )? b+c : b;
    return -c * std::cos(t / d * (PI_FOR_EASING / 2.0f)) + c + b;
}
inline float sineEaseOut(float t, float b, float c, float d) {
    if (d == 0.0f) return (t >=d )? b+c : b;
    return c * std::sin(t / d * (PI_FOR_EASING / 2.0f)) + b;
}
inline float sineEaseInOut(float t, float b, float c, float d) {
    if (d == 0.0f) return (t >=d )? b+c : b; // If duration is 0, and t is also 0, returns b. If t > 0, returns b+c.
    return -c / 2.0f * (std::cos(PI_FOR_EASING * t / d) - 1.0f) + b;
}

} // namespace easing

namespace normalized_easing {
inline float linear(float alpha) { return alpha; }
inline float expoEaseIn(float alpha) {
if (alpha == 0.0f) return 0.0f;
return std::pow(2.0f, 10.0f * (alpha - 1.0f));
}
// Add other normalized versions if needed
} // namespace normalized_easing

template<typename T>
T lerp(const T& start, const T& end, float alpha) {
return start + (end - start) * alpha;
}

} // namespace math

#endif