#ifndef COLLISION_SYSTEM_HPP
#define COLLISION_SYSTEM_HPP

#include "PhysicsTypes.hpp"
#include <vector>
#include <SFML/System/Vector2.hpp>
#include "Player.hpp" 
#include "PlatformBody.hpp" 
// i am not burying this comments, since the names are naming itself, i just noticed comments are dirty and fuck the book
namespace phys {

    struct CollisionEvent {
        float time = 1.0f;
        int axis = -1;
        const PlatformBody* hitPlatform = nullptr;
    };

    struct CollisionResolutionInfo {
        bool onGround = false;
        bool hitCeiling = false;
        bool hitWallLeft = false;
        bool hitWallRight = false;
        sf::Vector2f surfaceVelocity = {0.f, 0.f};
        const PlatformBody* groundPlatform = nullptr; 
    };

    class CollisionSystem {
    public:
        static CollisionResolutionInfo resolveCollisions(
            DynamicBody& dynamicBody,
            const std::vector<PlatformBody>& platformBodies,
            float deltaTime
        );

        static bool sweptAABB(
            const DynamicBody& body,
            const sf::Vector2f& displacement,
            const PlatformBody& platform,
            float maxTime,
            CollisionEvent& outCollisionEvent
        );

    private:
        static void applyCollisionResponse(
            DynamicBody& dynamicBody,
            const CollisionEvent& event,
            const PlatformBody& hitPlatform 
        );
    };

} 
#endif 