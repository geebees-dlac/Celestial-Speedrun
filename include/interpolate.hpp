#ifndef MATH_EASING_HPP // Renamed for clarity, or INTERPOLATE_HPP is fine
#define MATH_EASING_HPP

#include <cmath> // C++ header for math functions
#include <numbers> // C++20 for std::numbers::pi, otherwise define precisely
#include "PhysicsTypes.hpp" 
namespace math {

    // If not C++20 or if <numbers> is not available/preferred:
    // constexpr double PI = 3.14159265358979323846;
    // Or even better for the trig functions that use it, use std::numbers::pi if available
    // otherwise, ensure the PI constant is as precise as needed.
    // For many easing functions, PI itself might not be required in this header if
    // the functions are purely algebraic. If sine/cosine functions use it,
    // ensure <cmath> provides M_PI (non-standard but common) or use std::numbers::pi.

    // --- Robert Penner Easing Functions (t, b, c, d pattern) ---
    // t: current time, b: beginning value, c: change in value, d: duration

    namespace easing { // Changed class 'interpolate' to namespace 'easing'

        // No constructor/destructor needed for a namespace of free functions

        inline float linear(float t, float b, float c, float d) {
            if (d == 0.0f) return b + c; // Avoid division by zero, return end value
            return c * (t / d) + b;
        }

        // Exponential
        inline float expoEaseIn(float t, float b, float c, float d) {
            if (t == 0.0f) return b;
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

        // --- Add implementations for all your other easing functions here ---
        // Sine
        inline float sineEaseIn(float t, float b, float c, float d) {
            if (d == 0.0f) return b + c;
            return -c * std::cos(t / d * (std::numbers::pi_v<float> / 2.0f)) + c + b;
        }

        // ... and so on.

        // It's good practice to implement them inline in the header if they are small,
        // or declare them here and define them in a corresponding .cpp file if they are larger
        // or to reduce compile times for very large sets of functions. Given these are
        // simple math, inline is generally fine.


    } // namespace easing


    // --- Alternative: Normalized Easing Functions (0 to 1 range) ---
    // These functions take an alpha (0 to 1) and return an eased alpha (0 to 1)
    // You can then use this with a general lerp:
    // result = lerp(startVal, endVal, eased_alpha);
    // Or directly: result = startVal + (endVal - startVal) * eased_alpha;

    namespace normalized_easing {

        inline float linear(float alpha) {
            return alpha;
        }

        inline float expoEaseIn(float alpha) {
            if (alpha == 0.0f) return 0.0f;
            return std::pow(2.0f, 10.0f * (alpha - 1.0f));
        }

        // ... other normalized versions ...

    } // namespace normalized_easing

    // Generic linear interpolation for any type that supports T + T and T * float
    template<typename T>
    T lerp(const T& start, const T& end, float alpha) {
        return start + (end - start) * alpha;
    }


} // namespace math

#endif // MATH_EASING_HPP