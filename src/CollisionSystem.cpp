#include "CollisionSystem.hpp"
#include "dynamicBody.hpp"
#include "platformBody.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream> //debbugin lang neh para nimo gian ayieeee :)


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
    resolutionInfo.groundPlatform = dynamicBody.getGroundPlatform();


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
                
                // --- PLATFORM TYPE SPECIFIC FILTERING ---
                phys::bodyType type = currentEventDetails.hitPlatform->getType();

                if (type == phys::bodyType::platform) { // Your new one-way platform
                    // Player's AABB at the START of this iteration's sweep attempt
                    sf::FloatRect bodyAABBAtSweepStart = dynamicBody.getAABB();

                    // Player trying to drop while on THIS platform.
                    if (dynamicBody.isTryingToDropFromPlatform() &&
                        dynamicBody.getGroundPlatform() == currentEventDetails.hitPlatform) {
                        dynamicBody.setGroundPlatformTemporarilyIgnored(currentEventDetails.hitPlatform);
                        resolutionInfo.onGround = false; // No longer on ground if dropping 
                        dynamicBody.setGroundPlatform(nullptr);
                        // We want to ensure if it re-collides this iteration with something else, it's okay.
                        continue; //re-evaluate loop every new "block" formed
                    }

                    // Valid collision with a one-way phys::bodyType::platform:
                    // 1. Y-axis collision (axis == 1).
                    // 2. Player is moving downwards (currentVelocity.y > 0 for this sweep).
                    // 3. Player's feet were at or above the platform's top surface at the START of the current sweep movement.
                    //    We use the player's position *before* sweepVector * earliestCollisionTOI is applied.
                    // thanks @boytboyt30, i follow him on github, hes a friend of mine hes a classmate
                    if (!(currentEventDetails.axis == 1 &&
                          currentVelocity.y > 0.f &&
                          (bodyAABBAtSweepStart.top + bodyAABBAtSweepStart.height) <= (currentEventDetails.hitPlatform->getAABB().top + JUMP_THROUGH_TOLERANCE)
                       )) {
                        continue; // Skip this event, its not a landing/collision
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
            // --- DEPENETRATION for overlaps (TOI approx 0) ---
            if (earliestCollisionTOI < 1e-4f) { // Is an initial overlap
                sf::FloatRect bodyAABB = dynamicBody.getAABB();
                sf::FloatRect platAABB = hitPlatformInIter->getAABB();
                sf::Vector2f posCorrection(0.f, 0.f);

                // Using nearestCollisionEvent.axis determined by sweptAABB for static overlaps
                if (nearestCollisionEvent.axis == 1) { // Y-axis overlap
                    float bodyBottom = bodyAABB.top + bodyAABB.height;
                    float platTop = platAABB.top;
                    float bodyTop = bodyAABB.top;
                    float platBottom = platAABB.top + platAABB.height;

                    if (currentVelocity.y >= 0 && bodyBottom > platTop) { // Moving down 
                        posCorrection.y = -(bodyBottom - platTop) - DEPENETRATION_BIAS;
                    } else if (currentVelocity.y < 0 && bodyTop < platBottom) { // Moving up 
                        posCorrection.y = (platBottom - bodyTop) + DEPENETRATION_BIAS;
                    }
                } else { // X-axis overlap
                    float bodyRight = bodyAABB.left + bodyAABB.width;
                    float platLeft = platAABB.left;
                    float bodyLeft = bodyAABB.left;
                    float platRight = platAABB.left + platAABB.width;

                    if (currentVelocity.x >= 0 && bodyRight > platLeft) { // Moving right
                        posCorrection.x = -(bodyRight - platLeft) - DEPENETRATION_BIAS;
                    } else if (currentVelocity.x < 0 && bodyLeft < platRight) { // Moving left
                        posCorrection.x = (platRight - bodyLeft) + DEPENETRATION_BIAS;
                    }
                }
                // the overlap of wasd considered it past it not just through it
                dynamicBody.setPosition(dynamicBody.getPosition() + posCorrection);
                // After depenetration, the sweepVector from a TOI=0 collision doesn't move the body
                // as earliestCollisionTOI
            }

            // Apply movement up to the point of collision
            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector * earliestCollisionTOI);

            // Apply velocity response based on collision axis
            applyCollisionResponse(dynamicBody, nearestCollisionEvent, *hitPlatformInIter);

            // Update resolutionInfo based on this confirmed collision after response
            if (nearestCollisionEvent.axis == 1) { // Y-axis
                if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y > 0.f) { // Landed
                    resolutionInfo.onGround = true;
                    resolutionInfo.groundPlatform = hitPlatformInIter;
                    if (hitPlatformInIter->getType() == phys::bodyType::conveyorBelt) {
                        resolutionInfo.surfaceVelocity = hitPlatformInIter->getSurfaceVelocity();
                    } else {
                        resolutionInfo.surfaceVelocity = {0.f, 0.f};
                    }
                } else if (dynamicBody.getVelocity().y == 0.f && currentVelocity.y < 0.f) { // Hit ceiling
                    resolutionInfo.hitCeiling = true;
                    // If we hit a ceiling, we are not on ground with *that* platform. Could still be on ground from a previous iteration if it was a corner hit, but typical ceiling hits mean not on ground.
                    if (resolutionInfo.groundPlatform == hitPlatformInIter) { // Unlikely unless perfectly flat multi-collision, speedrunners gunna love this
                         resolutionInfo.onGround = false;
                         resolutionInfo.groundPlatform = nullptr;
                    }
                }
            } else { // X-axis
                if (dynamicBody.getVelocity().x == 0.f && currentVelocity.x > 0.f) resolutionInfo.hitWallRight = true;
                else if (dynamicBody.getVelocity().x == 0.f && currentVelocity.x < 0.f) resolutionInfo.hitWallLeft = true;
            }
            timeRemaining *= (1.0f - earliestCollisionTOI);
        } else { // No collision found for the rest of the time
            dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector);
            timeRemaining = 0;
        }
    }

    // After all iterations, update dynamicBody's own state based on final resolutionInfo
    dynamicBody.setOnGround(resolutionInfo.onGround);
    dynamicBody.setGroundPlatform(resolutionInfo.groundPlatform);    // Clear any temporary ignore once the whole physics step is done
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
        if (bodyRect.intersects(platRect)) {
            outCollisionEvent.time = 0.f;
            outCollisionEvent.hitPlatform = &platform;

            // Simplified Static Overlap Axis Determination:
            // Calculate overlap on each axis.
            // Axis with larger overlap is collision axis. Resolve Y first if overlaps are equal.
            float xOverlap = std::max(0.f, std::min(bodyRect.left + bodyRect.width, platRect.left + platRect.width) - std::max(bodyRect.left, platRect.left));
            float yOverlap = std::max(0.f, std::min(bodyRect.top + bodyRect.height, platRect.top + platRect.height) - std::max(bodyRect.top, platRect.top));

            if (yOverlap > 0 && yOverlap >= xOverlap) { // Prioritize Y if Y overlap exists and is greater or equal
                outCollisionEvent.axis = 1;
            } else if (xOverlap > 0) { // Else if X overlap exists
                outCollisionEvent.axis = 0;
            } else {
                 // Intersects reported true, but no positive overlap calculated.
                 // This can happen at exact touches or due to floating point issues.
                 // Default to Y axis to prevent player "sticking" to vertical surfaces if already on ground.
                 // Or, if velocity information from *previous* step was available, could use that.
                 // For now, a heuristic based on relative centers could be an alternative.
                 // Let's make a simple preference. (this is by chatgpt, they won here fuckers)
                 outCollisionEvent.axis = 1; // Prefer Y-axis for ambiguous static touches
            }
            return true;
        }
        return false;
    }

    // Swept Testing 
    sf::Vector2f invEntry, invExit;
    if (displacement.x > 0.f) {
        invEntry.x = platRect.left - (bodyRect.left + bodyRect.width);
        invExit.x  = (platRect.left + platRect.width) - bodyRect.left;
    } else {
        invEntry.x = (platRect.left + platRect.width) - bodyRect.left;
        invExit.x  = platRect.left - (bodyRect.left + bodyRect.width);
    }
    if (displacement.y > 0.f) {
        invEntry.y = platRect.top - (bodyRect.top + bodyRect.height);
        invExit.y  = (platRect.top + platRect.height) - bodyRect.top;
    } else {
        invEntry.y = (platRect.top + platRect.height) - bodyRect.top;
        invExit.y  = platRect.top - (bodyRect.top + bodyRect.height);
    }

    sf::Vector2f entryTime, exitTime;
    if (displacement.x == 0.f) {
        entryTime.x = (invEntry.x <= 0.f && invExit.x >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); // Check if already overlapping on X
        exitTime.x = std::numeric_limits<float>::infinity();
        if (!(bodyRect.left < platRect.left + platRect.width && bodyRect.left + bodyRect.width > platRect.left)) { // No X overlap
            entryTime.x = -std::numeric_limits<float>::infinity();
        }
    } else {
        entryTime.x = invEntry.x / displacement.x;
        exitTime.x  = invExit.x  / displacement.x;
    }
    if (displacement.y == 0.f) {
        entryTime.y = (invEntry.y <= 0.f && invExit.y >= 0.f) ? 0.f : -std::numeric_limits<float>::infinity(); // Check if already overlapping on Y
        exitTime.y = std::numeric_limits<float>::infinity();
        if (!(bodyRect.top < platRect.top + platRect.height && bodyRect.top + bodyRect.height > platRect.top)) { // No Y overlap
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
    } //adjusted it from the previous code by https://code.markrichards.ninja/sfml/sfml-platformer-in-less-than-1-million-lines-part-2
    // since its logic is wrong, it should be actualEntryTOI > actualExitTOI or actualEntryTOI >= maxTime or actualExitTOI <= 0.f or actualEntryTOI > 1.0f
    outCollisionEvent.time = actualEntryTOI; // Can be 0 if starting overlapped
    if (entryTime.x > entryTime.y) {
        outCollisionEvent.axis = 0;
    } else if (entryTime.y > entryTime.x) {
        outCollisionEvent.axis = 1;
    } else { 
        outCollisionEvent.axis = (std::abs(displacement.y) > std::abs(displacement.x)) ? 1 : 0; // Corner hitting
         if(displacement.x == 0.f && displacement.y == 0.f) outCollisionEvent.axis = 1; // Prefer Y for static corner
    }
    outCollisionEvent.hitPlatform = &platform;
    return true;
}


void CollisionSystem::applyCollisionResponse(
    DynamicBody& dynamicBody,
    const CollisionEvent& event,
    const PlatformBody& hitPlatform) // hitPlatform parameter is needed for its type for specific responses later
{
    sf::Vector2f vel = dynamicBody.getVelocity();
    // For solid platforms, always stop motion into them.
    // For one-way platforms, this response is only called if it's a valid collision (e.g., landing on top).
    if (event.axis == 0) { // Horizontal collision
        vel.x = 0.f;
    } else if (event.axis == 1) { // Vertical collision
        vel.y = 0.f;
    }
    dynamicBody.setVelocity(vel);
}

} 