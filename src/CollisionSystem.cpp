// CollisionSystem.cpp
#include "CollisionSystem.hpp"
#include "Player.hpp"
#include "PlatformBody.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream> 
#include "PhysicsTypes.hpp"


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
    resolutionInfo.hitCeiling = false;
    resolutionInfo.hitWallLeft = false;
    resolutionInfo.hitWallRight = false;
    resolutionInfo.surfaceVelocity = {0.f, 0.f};


    float timeRemaining = deltaTime;
    const int MAX_COLLISION_ITERATIONS = 5; // Iterative resolution attempts
    const float JUMP_THROUGH_TOLERANCE = 4.0f; // Pixels player's bottom can be inside platform top for one-way platform landing
    const float DEPENETRATION_BIAS = 0.01f;  // Small nudge out of collision
    const float MIN_TIME_STEP = 1e-5f; // Minimum time to process to avoid tiny steps due to precision


    dynamicBody.setGroundPlatformTemporarilyIgnored(nullptr); // Clear any temporary ignore from previous frame

    sf::Vector2f originalPlayerVelocity = dynamicBody.getVelocity(); // Store velocity at start of this tick

    for (int iter = 0; iter < MAX_COLLISION_ITERATIONS && timeRemaining > MIN_TIME_STEP; ++iter) {
        float earliestCollisionTOI = 1.0f + MIN_TIME_STEP; // Start slightly above 1.0 to ensure any valid TOI is less
        CollisionEvent nearestCollisionEvent;
        nearestCollisionEvent.time = earliestCollisionTOI; // Initialize nearest event time
        const PlatformBody* hitPlatformInIter = nullptr;

        sf::Vector2f currentFrameVelocity = dynamicBody.getVelocity(); // Velocity for *this iteration's* sweep
        sf::Vector2f sweepVector = currentFrameVelocity * timeRemaining;

        // Broadphase (already implicitly done by iterating all platforms)
        // Narrowphase
        for (const auto& platform : platformBodies) {
            if (platform.getType() == phys::bodyType::goal || platform.getType() == phys::bodyType::none || platform.getType() == phys::bodyType::trap || platform.getType() == phys::bodyType::portal) {
                continue;
            }
            if (&platform == dynamicBody.getGroundPlatformTemporarilyIgnored()) {
                continue;
            }

            // Optional: AABB check for the sweep before detailed sweptAABB
            sf::FloatRect dynamicBroadAABB = dynamicBody.getAABB();
            if (sweepVector.x < 0) dynamicBroadAABB.position.x += sweepVector.x;
            dynamicBroadAABB.size.x += std::abs(sweepVector.x);
            if (sweepVector.y < 0) dynamicBroadAABB.position.y += sweepVector.y;
            dynamicBroadAABB.size.y += std::abs(sweepVector.y);

            if (!dynamicBroadAABB.findIntersection(platform.getAABB())) {
                continue;
            }


            CollisionEvent currentEventDetails;
            if (sweptAABB(dynamicBody, sweepVector, platform, 1.0f, currentEventDetails)) { // maxTime for sweptAABB here is relative to sweepVector
                // Filter collisions for one-way platforms (type == platform)
                if (currentEventDetails.hitPlatform->getType() == phys::bodyType::platform) {
                    // Player must be moving downwards (or nearly static but overlapping from above)
                    // Collision must be on the Y-axis (top surface of platform)
                    // Player's feet must be above or very slightly into the platform's top surface at the START of the sweepVector for this iteration
                    sf::FloatRect bodyAABBAtSweepStart = dynamicBody.getAABB(); // Player's AABB before this iteration's sweepVector application
                    bool canLandOnOneWay = (currentEventDetails.axis == 1 && // Y-axis collision normal (hit top/bottom of platform)
                                         currentFrameVelocity.y >= -JUMP_THROUGH_TOLERANCE && // Player moving down, or very slightly up but overlapping
                                         (bodyAABBAtSweepStart.position.y + bodyAABBAtSweepStart.size.y) <= (currentEventDetails.hitPlatform->getAABB().position.y + JUMP_THROUGH_TOLERANCE));

                    // If player is trying to drop through this specific platform
                    if (dynamicBody.isTryingToDropFromPlatform() && dynamicBody.getGroundPlatform() == currentEventDetails.hitPlatform) {
                        dynamicBody.setGroundPlatformTemporarilyIgnored(currentEventDetails.hitPlatform);
                        resolutionInfo.onGround = false; // No longer on this ground
                        if (resolutionInfo.groundPlatform == currentEventDetails.hitPlatform) {
                           resolutionInfo.groundPlatform = nullptr;
                        }
                        dynamicBody.setGroundPlatform(nullptr);
                        continue; // Ignore this collision, try to fall through
                    }

                    if (!canLandOnOneWay) {
                        continue; // Not a valid landing on this one-way platform, ignore it
                    }
                }

                // Update nearest collision if this one is earlier
                if (currentEventDetails.time < nearestCollisionEvent.time) {
                    nearestCollisionEvent = currentEventDetails;
                    hitPlatformInIter = nearestCollisionEvent.hitPlatform; // Redundant but clear
                }
            }
        }

        // Process the nearest collision for this iteration
        if (hitPlatformInIter && nearestCollisionEvent.time < 1.0f + MIN_TIME_STEP) { // Check if a valid collision was found
             // Sanity check for TOI being within [0, 1] range relative to current sweepVector
            if (nearestCollisionEvent.time < 0.0f) nearestCollisionEvent.time = 0.0f;
            if (nearestCollisionEvent.time > 1.0f) nearestCollisionEvent.time = 1.0f;


            // Move player to the point of impact
            sf::Vector2f movementToCollision = sweepVector * nearestCollisionEvent.time;
            dynamicBody.setPosition(dynamicBody.getPosition() + movementToCollision);

            // Apply collision response (e.g., stop velocity along collision normal)
            // Store the velocity *before* response, useful for platform interaction checks.
            sf::Vector2f velocityBeforeResponse = dynamicBody.getVelocity();
            applyCollisionResponse(dynamicBody, nearestCollisionEvent, *hitPlatformInIter);
            sf::Vector2f velocityAfterResponse = dynamicBody.getVelocity();


            // Update resolution info based on the nature of the collision
            if (nearestCollisionEvent.axis == 1) { // Collision with a horizontal surface
                if (velocityBeforeResponse.y >= 0 && velocityAfterResponse.y == 0) { // Landed (was moving down or static, now Y velocity is zero)
                    resolutionInfo.onGround = true;
                    resolutionInfo.groundPlatform = hitPlatformInIter;
                    if (hitPlatformInIter->getType() == phys::bodyType::conveyorBelt) {
                        resolutionInfo.surfaceVelocity = hitPlatformInIter->getSurfaceVelocity();
                    } else {
                        resolutionInfo.surfaceVelocity = {0.f, 0.f}; // Reset if not conveyor
                    }
                } else if (velocityBeforeResponse.y < 0 && velocityAfterResponse.y == 0) { // Hit ceiling (was moving up, now Y velocity is zero)
                    resolutionInfo.hitCeiling = true;
                     // If somehow thought it was on ground with this platform, unset it.
                    if (resolutionInfo.groundPlatform == hitPlatformInIter) {
                        resolutionInfo.onGround = false;
                        resolutionInfo.groundPlatform = nullptr;
                    }
                }
            } else { // Collision with a vertical surface (axis == 0)
                if (velocityBeforeResponse.x > 0 && velocityAfterResponse.x == 0) {
                    resolutionInfo.hitWallRight = true;
                } else if (velocityBeforeResponse.x < 0 && velocityAfterResponse.x == 0) {
                    resolutionInfo.hitWallLeft = true;
                }
            }

            // If very small TOI (already overlapping or just touched), attempt depenetration
            if (nearestCollisionEvent.time < MIN_TIME_STEP) {
                sf::FloatRect bodyAABB = dynamicBody.getAABB(); // Re-get AABB after moving to TOI
                sf::FloatRect platAABB = hitPlatformInIter->getAABB();
                sf::Vector2f penetrationDepth = {0.f, 0.f};
                sf::Vector2f correction = {0.f, 0.f};

                // Calculate X penetration
                float xOverlap = (bodyAABB.position.x < platAABB.position.x) ?
                                 (bodyAABB.position.x + bodyAABB.size.x) - platAABB.position.x :
                                 (platAABB.position.x + platAABB.size.x) - bodyAABB.position.x;
                // Calculate Y penetration
                float yOverlap = (bodyAABB.position.y < platAABB.position.y) ?
                                 (bodyAABB.position.y + bodyAABB.size.y) - platAABB.position.y :
                                 (platAABB.position.y + platAABB.size.y) - bodyAABB.position.y;

                if (nearestCollisionEvent.axis == 1 && yOverlap > 0) { // Primary collision was Y
                    if (dynamicBody.getPosition().y + bodyAABB.size.y / 2.f < platAABB.position.y + platAABB.size.y / 2.f) { // Player center above platform center (hit top)
                        correction.y = -yOverlap - DEPENETRATION_BIAS; // Push player up
                    } else { // Player center below platform center (hit bottom)
                        correction.y = yOverlap + DEPENETRATION_BIAS;  // Push player down
                    }
                } else if (nearestCollisionEvent.axis == 0 && xOverlap > 0) { // Primary collision was X
                    if (dynamicBody.getPosition().x + bodyAABB.size.x / 2.f < platAABB.position.x + platAABB.size.x / 2.f) { // Player center left of platform center
                        correction.x = -xOverlap - DEPENETRATION_BIAS; // Push player left
                    } else { // Player center right of platform center
                        correction.x = xOverlap + DEPENETRATION_BIAS;  // Push player right
                    }
                }
                 // Apply depenetration only if a clear primary axis was found from SweptAABB and overlap exists on that axis
                if ( (nearestCollisionEvent.axis == 1 && std::abs(correction.y) > 1e-4f) ||
                     (nearestCollisionEvent.axis == 0 && std::abs(correction.x) > 1e-4f) ) {
                    dynamicBody.setPosition(dynamicBody.getPosition() + correction);
                }
            }

            // Update remaining time for this physics tick
            timeRemaining -= nearestCollisionEvent.time * timeRemaining; // timeRemaining * (1.0f - nearestCollisionEvent.time)
             if (timeRemaining < 0) timeRemaining = 0;

        } else { // No collision found in this iteration
            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector);
            timeRemaining = 0; // All remaining time consumed by free movement
        }
    }

    // Final update to dynamic body state based on resolution
    dynamicBody.setOnGround(resolutionInfo.onGround);
    dynamicBody.setGroundPlatform(resolutionInfo.groundPlatform);

    return resolutionInfo;
}


bool CollisionSystem::sweptAABB(
    const DynamicBody& body,
    const sf::Vector2f& displacement, // This is velocity * timeRemaining for the current iteration
    const PlatformBody& platform,
    float maxTime, // This should always be 1.0f as 'displacement' is the full potential move for this iteration
    CollisionEvent& outCollisionEvent)
{
    sf::FloatRect bodyRect = body.getAABB();
    sf::FloatRect platRect = platform.getAABB();

    outCollisionEvent.time = 2.0f; // Initialize to a value greater than 1.0f
    outCollisionEvent.axis = -1;
    outCollisionEvent.hitPlatform = nullptr;

    // Handle zero displacement case (static overlap check)
    if (std::abs(displacement.x) < 1e-5f && std::abs(displacement.y) < 1e-5f) {
        if (bodyRect.findIntersection(platRect)) {
            outCollisionEvent.time = 0.0f; // Immediate collision
            outCollisionEvent.hitPlatform = &platform;

            // Determine axis for static overlap: axis of MINIMUM penetration is preferred for depenetration
            float dx1 = (platRect.position.x + platRect.size.x) - bodyRect.position.x; // Right edge of plat - left edge of body
            float dx2 = (bodyRect.position.x + bodyRect.size.x) - platRect.position.x; // Right edge of body - left edge of plat
            float dy1 = (platRect.position.y + platRect.size.y) - bodyRect.position.y;   // Bottom edge of plat - top edge of body
            float dy2 = (bodyRect.position.y + bodyRect.size.y) - platRect.position.y;   // Bottom edge of body - top edge of plat

            float xOverlap = std::min(dx1, dx2);
            float yOverlap = std::min(dy1, dy2);

            if (xOverlap > 0 && yOverlap > 0) { // Actual overlap
                 if (xOverlap < yOverlap) { // Less penetration along X-axis implies Y-normal hit
                    outCollisionEvent.axis = 0; // Respond along X (normal is Y-axis of platform) is wrong, it means we separate along X
                                               // The axis should be the normal OF THE PLATFORM.
                                               // If xOverlap is smaller, the NORMAL IS ALONG X. So axis = 0.
                } else {
                    outCollisionEvent.axis = 1; // Normal is along Y. So axis = 1.
                }
            } else { // Should not happen if intersects is true, but as a fallback
                return false; // No clear overlap to determine axis
            }
            return true;
        }
        return false; // No static overlap
    }


    sf::Vector2f entryTime = {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
    sf::Vector2f exitTime = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

    // Calculate collision times for X axis
    if (std::abs(displacement.x) > 1e-5f) {
        if (displacement.x > 0.f) { // Moving Right
            entryTime.x = (platRect.position.x - (bodyRect.position.x + bodyRect.size.x)) / displacement.x;
            exitTime.x = ((platRect.position.x + platRect.size.x) - bodyRect.position.x) / displacement.x;
        } else { // Moving Left
            entryTime.x = ((platRect.position.x + platRect.size.x) - bodyRect.position.x) / displacement.x;
            exitTime.x = (platRect.position.x - (bodyRect.position.x + bodyRect.size.x)) / displacement.x;
        }
    } else { // Static in X: check for current overlap in X
        if (!(bodyRect.position.x + bodyRect.size.x <= platRect.position.x
            || bodyRect.position.x >= platRect.position.x + platRect.size.x)) { // Overlapping in X
            entryTime.x = -std::numeric_limits<float>::infinity(); // Can collide at any time during Y move
            exitTime.x = std::numeric_limits<float>::infinity();
        } // else, they are separate in X and not moving in X, so no X collision possible
    }

    // Calculate collision times for Y axis
    if (std::abs(displacement.y) > 1e-5f) {
        if (displacement.y > 0.f) { // Moving Down
            entryTime.y = (platRect.position.y - (bodyRect.position.y + bodyRect.size.y)) / displacement.y;
            exitTime.y = ((platRect.position.y + platRect.size.y) - bodyRect.position.y) / displacement.y;
        } else { // Moving Up
            entryTime.y = ((platRect.position.y + platRect.size.y) - bodyRect.position.y) / displacement.y;
            exitTime.y = (platRect.position.y - (bodyRect.position.y + bodyRect.size.y)) / displacement.y;
        }
    } else { // Static in Y: check for current overlap in Y
         if (!(bodyRect.position.y + bodyRect.size.y <= platRect.position.y
            || bodyRect.position.y >= platRect.position.y + platRect.size.y)) { // Overlapping in Y
            entryTime.y = -std::numeric_limits<float>::infinity();
            exitTime.y = std::numeric_limits<float>::infinity();
        }
    }
    
    // Times are normalized [0, 1] relative to the displacement for this iteration
    // Ensure entry times are less than exit times (if displacement was negative, they might be swapped)
    if (entryTime.x > exitTime.x) std::swap(entryTime.x, exitTime.x);
    if (entryTime.y > exitTime.y) std::swap(entryTime.y, exitTime.y);

    float firstEntry = std::max(entryTime.x, entryTime.y); // Time when swept AABBs first touch
    float lastExit = std::min(exitTime.x, exitTime.y);     // Time when swept AABBs last touch

    // No collision if:
    // - interval is invalid (firstEntry > lastExit)
    // - collision interval doesn't overlap with [0, 1] (or [0, maxTime] which should be 1 here)
    //   (firstEntry >= 1.0f means collision happens after full displacement)
    //   (lastExit <= 0.0f means collision happened before or at the start of displacement, or they are already separate)
    if (firstEntry > lastExit || firstEntry >= 1.0f || lastExit <= 0.0f) {
        return false; // No collision within this iteration's displacement
    }

    // A collision will occur
    outCollisionEvent.time = firstEntry;
    outCollisionEvent.hitPlatform = &platform;

    // Determine the collision normal (axis)
    // The axis where entryTime is GREATER determines the normal of the surface hit.
    // If entryTime.x > entryTime.y, it means it took longer to make contact on X-relevant surfaces
    // which implies Y-movement primarily led to the collision (or X-separation was greater).
    // The normal will be along the axis that defined 'firstEntry'.
    if (entryTime.x > entryTime.y) {
        // 'firstEntry' was 'entryTime.x'.
        // This means collision happened when X-bounds met. Normal is along X.
        outCollisionEvent.axis = 0;
        // Sanity check the relative velocity against this normal
        // If moving right (disp.x > 0), normal should be -X. If left, +X.
        // Not directly setting normal vector here, just axis of response.
    } else if (entryTime.y > entryTime.x) {
        // 'firstEntry' was 'entryTime.y'. Normal is along Y.
        outCollisionEvent.axis = 1;
    } else { // entryTime.x == entryTime.y (Corner hit or sliding perfectly aligned)
        // Resolve based on which component of displacement is "stronger" or by some other heuristic.
        // Using component of displacement, a larger component suggests that axis is "more responsible" for the collision.
        // However, for platformers, y-axis collisions (ground/ceiling) are often prioritized.
        // If moving perfectly diagonally into a corner, the choice is somewhat arbitrary without more info.
        // A common heuristic: if player is primarily moving vertically, treat as Y collision.
        if (std::abs(displacement.y) > std::abs(displacement.x) * 0.8f ) { // Prioritize Y if Y displacement significant
            outCollisionEvent.axis = 1;
        } else if (std::abs(displacement.x) > std::abs(displacement.y) * 0.8f) {
            outCollisionEvent.axis = 0;
        } else {
            // Fallback for very ambiguous corners, e.g., check overlaps
             // Determine axis for static overlap: axis of MINIMUM penetration is preferred for depenetration
            float dx1 = (platRect.position.x + platRect.size.x) - bodyRect.position.x; 
            float dx2 = (bodyRect.position.x + bodyRect.size.x) - platRect.position.x; 
            float dy1 = (platRect.position.y + platRect.size.y) - bodyRect.position.y;   
            float dy2 = (bodyRect.position.y + bodyRect.size.y) - platRect.position.y;  

            float xOverlap = std::min(dx1, dx2);
            float yOverlap = std::min(dy1, dy2);
            if (xOverlap < yOverlap) {
                outCollisionEvent.axis = 0;
            } else {
                outCollisionEvent.axis = 1;
            }
        }
    }
    return true;
}


void CollisionSystem::applyCollisionResponse(
    DynamicBody& dynamicBody,
    const CollisionEvent& event,
    const PlatformBody& hitPlatform)
{
    sf::Vector2f vel = dynamicBody.getVelocity();
    if (event.axis == 0) { // Hit a vertical surface, zero X velocity
        vel.x = 0.f;
    } else if (event.axis == 1) { // Hit a horizontal surface, zero Y velocity
        vel.y = 0.f;
    }
    dynamicBody.setVelocity(vel);
}

}