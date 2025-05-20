// CollisionSystem.cpp
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
    // resolutionInfo.groundPlatform = dynamicBody.getGroundPlatform(); 
    // ^ This line from your original code was already commented out by me previously.
    // Ground platform is determined by actual collision, so initializing it here directly from dynamicBody can be misleading.
    // It's better to set resolutionInfo.onGround and resolutionInfo.groundPlatform when a landing actually occurs.
    resolutionInfo.onGround = false; // Initialize to not on ground
    resolutionInfo.groundPlatform = nullptr; // Initialize to no ground platform

    float timeRemaining = deltaTime;
    const int MAX_COLLISION_ITERATIONS = 5;
    const float JUMP_THROUGH_TOLERANCE = 3.0f; // How far the framing of this shit on the top where it considers to land, will change this later
    const float DEPENETRATION_BIAS = 0.01f; // Small bias to push out fully ok book you have my thanks for this

    dynamicBody.setGroundPlatformTemporarilyIgnored(nullptr); // Clear ignore from previous tick

    for (int iter = 0; iter < MAX_COLLISION_ITERATIONS && timeRemaining > 1e-5f; ++iter) {
        float earliestCollisionTOI = 1.0f;
        CollisionEvent nearestCollisionEvent;
        const PlatformBody* hitPlatformInIter = nullptr;

        sf::Vector2f currentVelocity = dynamicBody.getVelocity(); // Velocity on the iteration 
        sf::Vector2f sweepVector = currentVelocity * timeRemaining;

        for (const auto& platform : platformBodies) {
            // --- MODIFICATION: Skip physical collision checks for goal platforms ---
            if (platform.getType() == phys::bodyType::goal) {
                continue; // Goal platforms are non-collidable for physics resolution
            }
            // --- END MODIFICATION ---

            if (platform.getType() == phys::bodyType::none || &platform == dynamicBody.getGroundPlatformTemporarilyIgnored()) {
                continue;
            }

            sf::FloatRect dynamicSweptAABB = dynamicBody.getAABB(); 
            if (sweepVector.x < 0) dynamicSweptAABB.position.x += sweepVector.x;
            dynamicSweptAABB.size.x += std::abs(sweepVector.x);
            if (sweepVector.y < 0) dynamicSweptAABB.position.y += sweepVector.y;
            dynamicSweptAABB.size.y += std::abs(sweepVector.y);

            if (!dynamicSweptAABB.findIntersection(platform.getAABB())) {
                continue;
            }

            CollisionEvent currentEventDetails;
            if (sweptAABB(dynamicBody, sweepVector, platform, 1.0f, currentEventDetails)) {
                
                // --- PLATFORM TYPE SPECIFIC FILTERING ---
                phys::bodyType type = currentEventDetails.hitPlatform->getType();

                if (type == phys::bodyType::platform) { // Your new one-way platform
                    sf::FloatRect bodyAABBAtSweepStart = dynamicBody.getAABB();

                    if (dynamicBody.isTryingToDropFromPlatform() &&
                        dynamicBody.getGroundPlatform() == currentEventDetails.hitPlatform) {
                        dynamicBody.setGroundPlatformTemporarilyIgnored(currentEventDetails.hitPlatform);
                        resolutionInfo.onGround = false; 
                        dynamicBody.setGroundPlatform(nullptr);
                        // also update the iteration's idea of groundPlatform since we're dropping
                        if (resolutionInfo.groundPlatform == currentEventDetails.hitPlatform) {
                            resolutionInfo.groundPlatform = nullptr;
                        }
                        continue; 
                    }
                    
                    if (!(currentEventDetails.axis == 1 &&
                          currentVelocity.y > 0.f &&
                          (bodyAABBAtSweepStart.position.y + bodyAABBAtSweepStart.size.y) <= (currentEventDetails.hitPlatform->getAABB().position.y + JUMP_THROUGH_TOLERANCE)
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
            if (earliestCollisionTOI < 1e-4f) { 
                sf::FloatRect bodyAABB = dynamicBody.getAABB();
                sf::FloatRect platAABB = hitPlatformInIter->getAABB();
                sf::Vector2f posCorrection(0.f, 0.f);

                if (nearestCollisionEvent.axis == 1) { 
                    float bodyBottom = bodyAABB.position.y + bodyAABB.size.y;
                    float platTop = platAABB.position.y;
                    float bodyTop = bodyAABB.position.y;
                    float platBottom = platAABB.position.y + platAABB.size.y;

                    if (currentVelocity.y >= 0 && bodyBottom > platTop) { 
                        posCorrection.y = -(bodyBottom - platTop) - DEPENETRATION_BIAS;
                    } else if (currentVelocity.y < 0 && bodyTop < platBottom) { 
                        posCorrection.y = (platBottom - bodyTop) + DEPENETRATION_BIAS;
                    }
                } else { 
                    float bodyRight = bodyAABB.position.x + bodyAABB.size.x;
                    float platLeft = platAABB.position.x;
                    float bodyLeft = bodyAABB.position.x;
                    float platRight = platAABB.position.x + platAABB.size.x;

                    if (currentVelocity.x >= 0 && bodyRight > platLeft) { 
                        posCorrection.x = -(bodyRight - platLeft) - DEPENETRATION_BIAS;
                    } else if (currentVelocity.x < 0 && bodyLeft < platRight) { 
                        posCorrection.x = (platRight - bodyLeft) + DEPENETRATION_BIAS;
                    }
                }
                dynamicBody.setPosition(dynamicBody.getPosition() + posCorrection);
            }

            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector * earliestCollisionTOI);
            applyCollisionResponse(dynamicBody, nearestCollisionEvent, *hitPlatformInIter);

            if (nearestCollisionEvent.axis == 1) { 
                if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y > 0.f) { 
                    resolutionInfo.onGround = true;
                    resolutionInfo.groundPlatform = hitPlatformInIter;
                    if (hitPlatformInIter->getType() == phys::bodyType::conveyorBelt) {
                        resolutionInfo.surfaceVelocity = hitPlatformInIter->getSurfaceVelocity();
                    } else {
                        resolutionInfo.surfaceVelocity = {0.f, 0.f};
                    }
                } else if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y < 0.f) { 
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
            timeRemaining *= (1.0f - earliestCollisionTOI);
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
    float maxTime,
    CollisionEvent& outCollisionEvent)
{
    sf::FloatRect bodyRect = body.getAABB();
    sf::FloatRect platRect = platform.getAABB();

    // Static Overlap (No Displacement)
    if (displacement.x == 0.f && displacement.y == 0.f) {
        if (bodyRect.findIntersection(platRect)) {
            outCollisionEvent.time = 0.f;
            outCollisionEvent.hitPlatform = &platform;

            float xOverlap = std::max(0.f, std::min(bodyRect.position.x + bodyRect.size.x, platRect.position.x + platRect.size.x) - std::max(bodyRect.position.x, platRect.position.x));
            float yOverlap = std::max(0.f, std::min(bodyRect.position.y + bodyRect.size.y, platRect.position.y + platRect.size.y) - std::max(bodyRect.position.y, platRect.position.y));

            if (yOverlap > 0 && yOverlap >= xOverlap) { 
                outCollisionEvent.axis = 1;
            } else if (xOverlap > 0) { 
                outCollisionEvent.axis = 0;
            } else {
                 outCollisionEvent.axis = 1; 
            }
            return true;
        }
        return false;
    }

    // Swept Testing 
    sf::Vector2f invEntry, invExit;
    if (displacement.x > 0.f) {
        invEntry.x = platRect.position.x - (bodyRect.position.x + bodyRect.size.x);
        invExit.x  = (platRect.position.x + platRect.size.x) - bodyRect.position.x;
    } else {
        invEntry.x = (platRect.position.x + platRect.size.x) - bodyRect.position.x;
        invExit.x  = platRect.position.x - (bodyRect.position.x + bodyRect.size.x);
    }
    if (displacement.y > 0.f) {
        invEntry.y = platRect.position.y - (bodyRect.position.y + bodyRect.size.y);
        invExit.y  = (platRect.position.y + platRect.size.y) - bodyRect.position.y;
    } else {
        invEntry.y = (platRect.position.y + platRect.size.y) - bodyRect.position.y;
        invExit.y  = platRect.position.y - (bodyRect.position.y + bodyRect.size.y);
    }

    sf::Vector2f entryTime, exitTime;
    if (displacement.x == 0.f) {
        entryTime.x = (invEntry.x <= 0.f && invExit.x >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); 
        exitTime.x = std::numeric_limits<float>::infinity();
        if (!(bodyRect.position.x < platRect.position.x + platRect.size.x && bodyRect.position.x + bodyRect.size.x > platRect.position.x)) { 
            entryTime.x = -std::numeric_limits<float>::infinity();
        }
    } else {
        entryTime.x = invEntry.x / displacement.x;
        exitTime.x  = invExit.x  / displacement.x;
    }
    if (displacement.y == 0.f) {
        entryTime.y = (invEntry.y <= 0.f && invExit.y >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); 
        exitTime.y = std::numeric_limits<float>::infinity();
        if (!(bodyRect.position.y < platRect.position.y + platRect.size.y && bodyRect.position.y + bodyRect.size.y > platRect.position.y)) { 
             entryTime.y = -std::numeric_limits<float>::infinity();
        }
    } else {
        entryTime.y = invEntry.y / displacement.y;
        exitTime.y  = invExit.y  / displacement.y;
    }

    if (entryTime.x > exitTime.x) std::swap(entryTime.x, exitTime.x);
    if (entryTime.y > exitTime.y) std::swap(entryTime.y, exitTime.y);

    float actualEntryTOI = std::max(entryTime.x, entryTime.y);
    float actualExitTOI  = std::min(exitTime.x, exitTime.y);

    if (actualEntryTOI > actualExitTOI || actualEntryTOI >= maxTime || actualExitTOI <= 0.f || actualEntryTOI > 1.0f) {
        return false;
    } 
    outCollisionEvent.time = actualEntryTOI; 
    if (entryTime.x > entryTime.y) {
        outCollisionEvent.axis = 0;
    } else if (entryTime.y > entryTime.x) {
        outCollisionEvent.axis = 1;
    } else { 
        outCollisionEvent.axis = (std::abs(displacement.y) > std::abs(displacement.x)) ? 1 : 0; 
         if(displacement.x == 0.f && displacement.y == 0.f) outCollisionEvent.axis = 1; 
    }
    outCollisionEvent.hitPlatform = &platform;
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