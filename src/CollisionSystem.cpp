#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream> //debbugin lang neh para nimo gian ayieeee :)
#include "PhysicsTypes.hpp" // Added for phys::bodyType::goal comparison


//mao ni inyong legend placing it here since most of the physics if not all is here
// what is AABB? Axis-Aligned Bounding Box. this is the hitbox of the player
// what is sweptAABB? this is the hitbox of the player when it is moving, this is used to check if the player is colliding with the platform
// what is TOI? Time of Impact. this is the time it takes for the player to hit the platform

namespace phys {

CollisionResolutionInfo CollisionSystem::resolveCollisions(
    DynamicBody& dynamicBody,
    const std::vector<PlatformBody>& platformBodies,
    float deltaTime)
{
    CollisionResolutionInfo resolutionInfo;
    resolutionInfo.onGround = false;
    resolutionInfo.groundPlatform = nullptr;

    float timeRemaining = deltaTime;
    const int MAX_COLLISION_ITERATIONS = 5;
    const float JUMP_THROUGH_TOLERANCE = 3.0f;
    const float DEPENETRATION_BIAS = 0.01f;

    dynamicBody.setGroundPlatformTemporarilyIgnored(nullptr);

    for (int iter = 0; iter < MAX_COLLISION_ITERATIONS && timeRemaining > 1e-5f; ++iter) {
        float earliestCollisionTOI = 1.0f;
        CollisionEvent nearestCollisionEvent;
        const PlatformBody* hitPlatformInIter = nullptr;

        sf::Vector2f currentVelocity = dynamicBody.getVelocity();
        sf::Vector2f sweepVector = currentVelocity * timeRemaining;

        for (const auto& platform : platformBodies) {
            if (platform.getType() == phys::bodyType::goal) {
                continue;
            }

            if (platform.getType() == phys::bodyType::none || &platform == dynamicBody.getGroundPlatformTemporarilyIgnored()) {
                continue;
            }

            sf::FloatRect dynamicSweptAABB = dynamicBody.getAABB();
            if (sweepVector.x < 0) dynamicSweptAABB.left += sweepVector.x;
            dynamicSweptAABB.width += std::abs(sweepVector.x);
            if (sweepVector.y < 0) dynamicSweptAABB.top += sweepVector.y;
            dynamicSweptAABB.height += std::abs(sweepVector.y);

            if (!dynamicSweptAABB.intersects(platform.getAABB())) {
                continue;
            }

            CollisionEvent currentEventDetails;
            if (sweptAABB(dynamicBody, sweepVector, platform, 1.0f, currentEventDetails)) {

                phys::bodyType type = currentEventDetails.hitPlatform->getType();

                if (type == phys::bodyType::platform) {
                    sf::FloatRect bodyAABBAtSweepStart = dynamicBody.getAABB();

                    if (dynamicBody.isTryingToDropFromPlatform() &&
                        dynamicBody.getGroundPlatform() == currentEventDetails.hitPlatform) {
                        dynamicBody.setGroundPlatformTemporarilyIgnored(currentEventDetails.hitPlatform);
                        resolutionInfo.onGround = false;
                        dynamicBody.setGroundPlatform(nullptr);
                        if (resolutionInfo.groundPlatform == currentEventDetails.hitPlatform) {
                            resolutionInfo.groundPlatform = nullptr;
                        }
                        continue;
                    }

                    if (!(currentEventDetails.axis == 1 &&
                          currentVelocity.y >= 0.f && // Changed to >= 0 to allow landing even if y_vel is 0 but overlap occurs
                          (bodyAABBAtSweepStart.top + bodyAABBAtSweepStart.height) <= (currentEventDetails.hitPlatform->getAABB().top + JUMP_THROUGH_TOLERANCE)
                       )) {
                        continue;
                    }
                }

                if (currentEventDetails.time < earliestCollisionTOI) {
                    earliestCollisionTOI = currentEventDetails.time;
                    nearestCollisionEvent = currentEventDetails;
                    hitPlatformInIter = nearestCollisionEvent.hitPlatform;
                }
            }
        }


        if (hitPlatformInIter) {
            if (earliestCollisionTOI < 1e-4f && earliestCollisionTOI >= 0.f) { // Ensure TOI is not negative
                sf::FloatRect bodyAABB = dynamicBody.getAABB();
                sf::FloatRect platAABB = hitPlatformInIter->getAABB();
                sf::Vector2f posCorrection(0.f, 0.f);

                if (nearestCollisionEvent.axis == 1) {
                    float bodyBottom = bodyAABB.top + bodyAABB.height;
                    float platTop = platAABB.top;
                    float bodyTop = bodyAABB.top;
                    float platBottom = platAABB.top + platAABB.height;

                    if (bodyBottom > platTop && bodyTop < platTop) { // Penetrating from top
                        posCorrection.y = -(bodyBottom - platTop) - DEPENETRATION_BIAS;
                    } else if (bodyTop < platBottom && bodyBottom > platBottom) { // Penetrating from bottom
                        posCorrection.y = (platBottom - bodyTop) + DEPENETRATION_BIAS;
                    }
                } else {
                    float bodyRight = bodyAABB.left + bodyAABB.width;
                    float platLeft = platAABB.left;
                    float bodyLeft = bodyAABB.left;
                    float platRight = platAABB.left + platAABB.width;

                    if (bodyRight > platLeft && bodyLeft < platLeft) { // Penetrating from left
                        posCorrection.x = -(bodyRight - platLeft) - DEPENETRATION_BIAS;
                    } else if (bodyLeft < platRight && bodyRight > platRight) { // Penetrating from right
                        posCorrection.x = (platRight - bodyLeft) + DEPENETRATION_BIAS;
                    }
                }
                dynamicBody.setPosition(dynamicBody.getPosition() + posCorrection);
            }

            // Ensure TOI is non-negative before moving
            float actualMoveTime = std::max(0.f, earliestCollisionTOI);
            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector * actualMoveTime);
            applyCollisionResponse(dynamicBody, nearestCollisionEvent, *hitPlatformInIter);

            if (nearestCollisionEvent.axis == 1) {
                if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y >= 0.f) { // Player was moving down or static, now stopped by a Y-collision
                    resolutionInfo.onGround = true;
                    resolutionInfo.groundPlatform = hitPlatformInIter;
                    if (hitPlatformInIter->getType() == phys::bodyType::conveyorBelt) {
                        resolutionInfo.surfaceVelocity = hitPlatformInIter->getSurfaceVelocity();
                    } else {
                        resolutionInfo.surfaceVelocity = {0.f, 0.f};
                    }
                } else if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y < 0.f) { // Player was moving up, now stopped by a Y-collision
                    resolutionInfo.hitCeiling = true;
                    if (resolutionInfo.groundPlatform == hitPlatformInIter) {
                         resolutionInfo.onGround = false;
                         resolutionInfo.groundPlatform = nullptr;
                    }
                }
            } else {
                if (dynamicBody.getVelocity().x == 0.f && currentVelocity.x > 0.f) resolutionInfo.hitWallRight = true;
                else if (dynamicBody.getVelocity().x == 0.f && currentVelocity.x < 0.f) resolutionInfo.hitWallLeft = true;
            }
            timeRemaining *= (1.0f - actualMoveTime);
            // Ensure timeRemaining is not negative due to float precision
            if (timeRemaining < 0.f) timeRemaining = 0.f;

        } else {
            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector);
            timeRemaining = 0;
        }
    }

    dynamicBody.setOnGround(resolutionInfo.onGround);
    dynamicBody.setGroundPlatform(resolutionInfo.groundPlatform);
    dynamicBody.setGroundPlatformTemporarilyIgnored(nullptr);


    return resolutionInfo;
}

bool CollisionSystem::sweptAABB(
    const DynamicBody& body,
    const sf::Vector2f& displacement,
    const PlatformBody& platform,
    float maxTime, // maxTime is effectively the fraction of the remaining deltaTime for this sweep iteration, typically 1.0f
    CollisionEvent& outCollisionEvent)
{
    sf::FloatRect bodyRect = body.getAABB();
    sf::FloatRect platRect = platform.getAABB();

    // Check for initial static overlap (if displacement is zero or effectively zero)
    if (std::abs(displacement.x) < 1e-5f && std::abs(displacement.y) < 1e-5f) {
        if (bodyRect.intersects(platRect)) {
            outCollisionEvent.time = 0.f;
            outCollisionEvent.hitPlatform = &platform;

            // Determine axis for static overlap (this is a simplification, true normal needs more info)
            // A common heuristic is to find the axis of least penetration
            float xOverlap = std::min(bodyRect.left + bodyRect.width, platRect.left + platRect.width) - std::max(bodyRect.left, platRect.left);
            float yOverlap = std::min(bodyRect.top + bodyRect.height, platRect.top + platRect.height) - std::max(bodyRect.top, platRect.top);

            if (xOverlap > 0 && yOverlap > 0) { // True overlap
                 // If Y overlap is slightly greater or they are very close, prioritize Y (ground/ceiling)
                if (yOverlap < xOverlap) { // If xOverlap is significantly more, it's a side collision
                     outCollisionEvent.axis = 0; //  Less penetration in Y than X implies primary contact is vertical. Normal is horizontal (X-axis)
                } else {
                     outCollisionEvent.axis = 1; // Less penetration in X than Y implies primary contact is horizontal. Normal is vertical (Y-axis)
                }
            } else { // Touching but not fully overlapping, or error in overlap calculation
                return false;
            }
            return true;
        }
        return false;
    }

    // Swept Testing
    sf::Vector2f invEntry, invExit; // Distances to sides of the minkowski sum rectangle
    // Distance from dynamic body's relevant edge to static body's relevant edge
    if (displacement.x > 0.f) { // Moving right
        invEntry.x = platRect.left - (bodyRect.left + bodyRect.width);
        invExit.x  = (platRect.left + platRect.width) - bodyRect.left;
    } else { // Moving left (or static in x)
        invEntry.x = (platRect.left + platRect.width) - bodyRect.left;
        invExit.x  = platRect.left - (bodyRect.left + bodyRect.width);
    }

    if (displacement.y > 0.f) { // Moving down
        invEntry.y = platRect.top - (bodyRect.top + bodyRect.height);
        invExit.y  = (platRect.top + platRect.height) - bodyRect.top;
    } else { // Moving up (or static in y)
        invEntry.y = (platRect.top + platRect.height) - bodyRect.top;
        invExit.y  = platRect.top - (bodyRect.top + bodyRect.height);
    }

    sf::Vector2f entryTime, exitTime; // Time of entry and exit for each axis

    if (std::abs(displacement.x) < 1e-5f) { // Effectively not moving in X
        entryTime.x = (invEntry.x <= 0.f && invExit.x >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); // Check for overlap in X
        exitTime.x = std::numeric_limits<float>::infinity();
        // If not overlapping in X, no collision possible by X movement
        if (!(bodyRect.left < platRect.left + platRect.width && bodyRect.left + bodyRect.width > platRect.left)) {
             entryTime.x = -std::numeric_limits<float>::infinity(); // no X overlap means can't hit X-wise unless moving.
        }
    } else {
        entryTime.x = invEntry.x / displacement.x;
        exitTime.x  = invExit.x  / displacement.x;
    }

    if (std::abs(displacement.y) < 1e-5f) { // Effectively not moving in Y
        entryTime.y = (invEntry.y <= 0.f && invExit.y >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); // Check for overlap in Y
        exitTime.y = std::numeric_limits<float>::infinity();
        // If not overlapping in Y, no collision possible by Y movement
         if (!(bodyRect.top < platRect.top + platRect.height && bodyRect.top + bodyRect.height > platRect.top)) {
             entryTime.y = -std::numeric_limits<float>::infinity();
        }
    } else {
        entryTime.y = invEntry.y / displacement.y;
        exitTime.y  = invExit.y  / displacement.y;
    }

    // Ensure entry times are before exit times
    if (entryTime.x > exitTime.x) std::swap(entryTime.x, exitTime.x);
    if (entryTime.y > exitTime.y) std::swap(entryTime.y, exitTime.y);

    // Time of first contact (actual entry) and last contact (actual exit) for the swept AABB
    float actualEntryTOI = std::max(entryTime.x, entryTime.y);
    float actualExitTOI  = std::min(exitTime.x, exitTime.y);

    // No collision if entry is after exit, or entry is after maxTime, or exit is before zero, or entry greater than 1.0 (full sweep)
    if (actualEntryTOI > actualExitTOI || actualEntryTOI >= maxTime || actualExitTOI < 0.f || actualEntryTOI > 1.0f ) { //Ensure TOI is within [0, 1) or [0, maxTime)
        return false;
    }

    // Collision occurs
    outCollisionEvent.time = actualEntryTOI;
    outCollisionEvent.hitPlatform = &platform;

    // Determine collision axis (normal of the surface hit)
    // The axis of collision corresponds to the normal of the surface of 'platform' that 'body' hits.
    // This happens along the axis whose 'entryTime' component contributed to 'actualEntryTOI'.
    if (entryTime.x > entryTime.y) {
        // Collision is primarily along the X-axis normal (hit a vertical side of the platform).
        // This means invEntry.x determined the time. Player velocity_x should be affected.
        outCollisionEvent.axis = 0;
    } else if (entryTime.y > entryTime.x) {
        // Collision is primarily along the Y-axis normal (hit a horizontal top/bottom of the platform).
        // This means invEntry.y determined the time. Player velocity_y should be affected.
        outCollisionEvent.axis = 1;
    } else { // entryTime.x == entryTime.y (corner hit or sliding along an edge that is perfectly aligned)
        // Tie-breaking:
        // If displacement.x is much larger, likely an X-collision normal, etc.
        // Or, more simply, based on penetration depth of displacement on each axis (if it were to complete)
        // This tie-breaking can be critical.
        // Consider relative speeds or intended direction. For platformers, Y often takes precedence for ground.
        // A simple heuristic:
        if (std::abs(displacement.x * (platRect.height + bodyRect.height)) > std::abs(displacement.y * (platRect.width + bodyRect.width))) { // Weighted by "other" dimension
            outCollisionEvent.axis = 0;
        } else {
            outCollisionEvent.axis = 1;
        }
        // If displacement is zero (static case already handled), this block shouldn't be primary.
        // If player moving exactly diagonally into a corner, this also fires.
    }

    return true;
}


void CollisionSystem::applyCollisionResponse(
    DynamicBody& dynamicBody,
    const CollisionEvent& event,
    const PlatformBody& hitPlatform)
{
    sf::Vector2f vel = dynamicBody.getVelocity();
    if (event.axis == 0) {
        vel.x = 0.f;
    } else if (event.axis == 1) {
        vel.y = 0.f;
    }
    dynamicBody.setVelocity(vel);
}

}