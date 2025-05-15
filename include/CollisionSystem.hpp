#ifndef COLLISION_SYSTEM_HPP
#define COLLISION_SYSTEM_HPP
#include "PhysicsTypes.hpp" // Or your chosen name
#include <cstdlib>
#include <algorithm>
// #include <cstdbool> // Not strictly needed if using C++ bool
#include <cstdio>
#include <vector> // For example
#include <iostream> // For debugging, usually remove for final

// Forward declare SFML types if needed and SFML headers are heavy
// For sf::Vector2f, including <SFML/System/Vector2.hpp> is light.
#include <SFML/System/Vector2.hpp>

// Assume these are your class definitions
#include <dynamicBody.hpp>
#include <platformBody.hpp>

namespace phys {

    // Describes a single collision event
    struct CollisionEvent {
        float time = 1.0f;         // Time of impact (0.0 to 1.0 within the frame/timeslice)
        int axis = -1;             // 0 for X, 1 for Y, -1 for no collision
        PlatformBody* hitPlatform = nullptr; // Pointer to the platform that was hit (optional, but useful)
                                            // Or an ID, or a copy of relevant info
        // float surfaceNormalX, surfaceNormalY; // Alternative to 'axis' for more complex shapes
    };

    // Information about resolved collisions for a dynamic body in a given update
    struct CollisionResolutionInfo {
        bool onGround = false;
        bool hitCeiling = false;
        bool hitWallLeft = false;
        bool hitWallRight = false;
        sf::Vector2f surfaceVelocity = {0.f, 0.f}; // e.g., from conveyor belt
        PlatformBody* groundPlatform = nullptr; // If onGround, what platform?
        // Add any other relevant info from the collision
    };

    // If platformBody has these, no need to duplicate:
    // enum BodyType { ... };
    // struct PlatformSpecificInfo {
    //     BodyType type;
    //     float surfaceVelocityMagnitude; // For conveyors
    // };


    class CollisionSystem {
    public:
        // No constructor/destructor needed if all static or pure utility

        // Checks and resolves collisions for a single dynamicBody against a list of platformBodies.
        // Returns info about what happened.
        static CollisionResolutionInfo resolveCollisions(
            DynamicBody& dynamicBody,                           // Pass by reference
            const std::vector<PlatformBody>& platformBodies,   // Pass by const reference
            float deltaTime                                    // The full time step for this update
        );

        // AABB sweep test. Returns time of impact (toi) from 0 to 1 (relative to swept distance).
        // Fills outCollision details if a collision occurs within the sweep.
        // Returns true if collision, false otherwise.
        // 'maxTime' is the remaining fraction of the deltaTime we are testing (usually 1.0 initially)
        static bool sweptAABB(
            const DynamicBody& body,         // What is moving
            const sf::Vector2f& velocity,   // How it's moving in this sub-step
            const PlatformBody& platform,   // What it might hit
            float maxTime,                  // Don't consider collisions beyond this time fraction
            CollisionEvent& outCollisionEvent // Output: time of impact and axis
        );

    private:
        // Helper to resolve a single confirmed collision
        static void applyCollisionResponse(
            DynamicBody& dynamicBody,
            const CollisionEvent& event,
            const PlatformBody& hitPlatform // To get platform-specific properties like surface velocity
        );
    };

} // namespace phys

#endif //COLLISION_SYSTEM_HPP