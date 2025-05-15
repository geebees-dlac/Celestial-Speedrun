#include "CollisionSystem.hpp"
#include "dynamicBody.hpp"   // For DynamicBody definition
#include "platformBody.hpp"  // For PlatformBody definition
#include <SFML/Graphics/Rect.hpp> // For sf::FloatRect, getExpandedBy
#include <limits>       // For std::numeric_limits
#include <algorithm>    // For std::max, std::min

namespace phys {

CollisionResolutionInfo CollisionSystem::resolveCollisions(
    DynamicBody& dynamicBody, // Non-const ref to use setters
    const std::vector<PlatformBody>& platformBodies,
    float deltaTime)
{
    CollisionResolutionInfo resolutionInfo;
    // Keep track of original position before movement attempts within this frame.
    // The dynamicBody's m_lastPosition is from the *previous* frame's end.
    sf::Vector2f frameStartPosition = dynamicBody.getPosition();


    float timeRemaining = deltaTime; // Resolve over the full deltaTime. SweptAABB times are fractions of this.

    const int MAX_COLLISION_ITERATIONS = 5; // Iterative resolution

    for (int iter = 0; iter < MAX_COLLISION_ITERATIONS && timeRemaining > 0.0001f * deltaTime; ++iter) {
        float earliestCollisionTOI = 1.0f; // Time of impact (0 to 1 for the *current sweep segment*)
        CollisionEvent nearestCollisionEvent; // Details of the nearest collision
        const PlatformBody* hitPlatform = nullptr;

        sf::Vector2f currentVelocity = dynamicBody.getVelocity(); // Velocity for this sweep
        sf::Vector2f sweepVector = currentVelocity * timeRemaining; // Potential movement in remaining time

        // Narrowphase: Find the earliest collision in this iteration's remaining time
        for (const auto& platform : platformBodies) {
            // Broadphase check (simple expanded AABB test)
            // Make sure your DynamicBody and PlatformBody have getAABB()
            sf::FloatRect dynamicAABB = dynamicBody.getAABB();
            sf::FloatRect platformAABB = platform.getAABB();

            // Create a "swept" AABB for the dynamic body
            sf::FloatRect sweptDynamicAABB = dynamicAABB;
            if (sweepVector.x < 0) sweptDynamicAABB.left += sweepVector.x;
            sweptDynamicAABB.width += std::abs(sweepVector.x);
            if (sweepVector.y < 0) sweptDynamicAABB.top += sweepVector.y;
            sweptDynamicAABB.height += std::abs(sweepVector.y);

            if (!sweptDynamicAABB.intersects(platformAABB)) {
                continue; // No possible collision with this sweep
            }

            CollisionEvent currentEventDetails;
            // Call sweptAABB with velocity for *this iteration's remaining time*
            // Note: sweptAABB's 'velocity' param is total displacement for the segment it checks
            if (sweptAABB(dynamicBody, sweepVector, platform, 1.0f /*maxTime is 1.0 relative to sweepVector*/, currentEventDetails)) {
                if (currentEventDetails.time < earliestCollisionTOI) {
                    earliestCollisionTOI = currentEventDetails.time;
                    nearestCollisionEvent = currentEventDetails; // Copy details
                    hitPlatform = &platform; // Store pointer
                }
            }
        }

        // Move dynamicBody by the time until the first collision (or full remaining time if no collision)
        // earliestCollisionTOI is a fraction of the current 'sweepVector'
        dynamicBody.setPosition(dynamicBody.getPosition() + sweepVector * earliestCollisionTOI);

        if (hitPlatform) { // A collision occurred in this iteration
            applyCollisionResponse(dynamicBody, nearestCollisionEvent, *hitPlatform); // Modifies dynamicBody's velocity

            // Update resolutionInfo based on this specific collision
            // (This part of the logic for `resolutionInfo` needs refinement to accurately reflect contact)
            // Example for simple vertical collision determination:
            if (nearestCollisionEvent.axis == 1) { // Y-axis
                if (currentVelocity.y > 0 && dynamicBody.getVelocity().y <= 0.001f) { // Was moving down, now stopped or barely moving
                    resolutionInfo.onGround = true;
                    resolutionInfo.groundPlatform = hitPlatform; // Non-const version of hitPlatform if needed
                    // if (hitPlatform->getType() == bodyType::conveyorBelt) { // Conveyor logic might go here or in main loop
                    //    resolutionInfo.surfaceVelocity = hitPlatform->getSurfaceVelocity();
                    // }
                } else if (currentVelocity.y < 0 && dynamicBody.getVelocity().y >= -0.001f) {
                    resolutionInfo.hitCeiling = true;
                }
            } else if (nearestCollisionEvent.axis == 0) { // X-axis
                 if (currentVelocity.x > 0 && dynamicBody.getVelocity().x <= 0.001f) resolutionInfo.hitWallRight = true; // Hit wall on its right
                 else if (currentVelocity.x < 0 && dynamicBody.getVelocity().x >= -0.001f) resolutionInfo.hitWallLeft = true; // Hit wall on its left
            }
            timeRemaining -= timeRemaining * earliestCollisionTOI; // Reduce remaining time by consumed fraction
        } else {
            // No collision for the rest of the time step
            timeRemaining = 0; // All remaining time consumed by movement
        }
    }
     // After iterations, if any time remained and iterations maxed out,
     // this could indicate a stuck state.
     // The current loop structure already moves by the remaining sweepVector if no hitPlatform, so this is fine.


    // Optional: Update DynamicBody's internal m_onGround, etc. flags
    // based on the final resolutionInfo.
    // dynamicBody.setOnGround(resolutionInfo.onGround);
    // dynamicBody.setHitCeiling(resolutionInfo.hitCeiling);
    // ...
    return resolutionInfo;
}

// SweptAABB function: Checks if a moving body will collide with a static platform
// The `velocity` parameter here is the total displacement vector for the time segment being checked.
// `maxTime` (now 1.0f in resolveCollisions call) means the collision must occur within this displacement.
bool CollisionSystem::sweptAABB(
    const DynamicBody& body,
    const sf::Vector2f& displacement, // This is the actual sweep vector (e.g., velocity * sub_deltaTime)
    const PlatformBody& platform,
    float maxTime, // Usually 1.0 when `displacement` is for the current sub-step
    CollisionEvent& outCollisionEvent)
{
    sf::FloatRect bodyRect = body.getAABB();
    sf::FloatRect platRect = platform.getAABB();

    if (displacement.x == 0.f && displacement.y == 0.f) return false; // Not moving

    // Calculate distances to the near and far edges of the platform
    sf::Vector2f invEntry, invExit;
    if (displacement.x > 0.f) {
        invEntry.x = platRect.left - (bodyRect.left + bodyRect.width);
        invExit.x = (platRect.left + platRect.width) - bodyRect.left;
    } else {
        invEntry.x = (platRect.left + platRect.width) - bodyRect.left;
        invExit.x = platRect.left - (bodyRect.left + bodyRect.width);
    }
    if (displacement.y > 0.f) {
        invEntry.y = platRect.top - (bodyRect.top + bodyRect.height);
        invExit.y = (platRect.top + platRect.height) - bodyRect.top;
    } else {
        invEntry.y = (platRect.top + platRect.height) - bodyRect.top;
        invExit.y = platRect.top - (bodyRect.top + bodyRect.height);
    }

    // Calculate time of collision (entry) and time of exit for each axis
    sf::Vector2f entryTime, exitTime;
    if (displacement.x == 0.f) {
        entryTime.x = (invEntry.x > 0 || invExit.x < 0) ? -std::numeric_limits<float>::infinity() : 0.f; // Check for overlap if not moving on X
        exitTime.x = std::numeric_limits<float>::infinity();
    } else {
        entryTime.x = invEntry.x / displacement.x;
        exitTime.x = invExit.x / displacement.x;
    }
    if (displacement.y == 0.f) {
        entryTime.y = (invEntry.y > 0 || invExit.y < 0) ? -std::numeric_limits<float>::infinity() : 0.f; // Check for overlap if not moving on Y
        exitTime.y = std::numeric_limits<float>::infinity();
    } else {
        entryTime.y = invEntry.y / displacement.y;
        exitTime.y = invExit.y / displacement.y;
    }

    // Ensure entry times are not swapped with exit times
    if (entryTime.x > exitTime.x) std::swap(entryTime.x, exitTime.x);
    if (entryTime.y > exitTime.y) std::swap(entryTime.y, exitTime.y);


    // Find the actual earliest time of entry and latest time of entry across both axes
    float actualEntryTOI = std::max(entryTime.x, entryTime.y);
    // Find the actual earliest time of exit
    float actualExitTOI = std::min(exitTime.x, exitTime.y);


    // No collision if:
    // - The time of entry is after the time of exit
    // - The entry time is negative (moving away and not already overlapping)
    // - The entry time is greater than maxTime (collision happens too late for this step)
    if (actualEntryTOI > actualExitTOI || actualEntryTOI < 0.f || actualEntryTOI > maxTime) {
         // Also, consider if already overlapping and moving away on one axis but still overlapping on other.
        // A common initial check: if (bodyRect.intersects(platRect)) handle penetration. Swept for new penetrations.
        // For this simple swept: If not initially overlapping deeply and actualEntryTOI < 0, often means moving away.
        // A more robust check is to see if they are separated along any axis at t=0 if actualEntryTOI < 0
        if (actualEntryTOI < 0.f && bodyRect.intersects(platRect)) { // Already overlapping, moving away from one entry point
             // Potentially complex, treat as toi=0 if significant initial overlap is not handled elsewhere
             // For now, this swept test might report this as no *new* collision if actualEntryTOI becomes >0 after sorting times
             // Let's refine: No new collision IF entry is beyond maxTime OR they start separated and entry is negative
        } else if (actualEntryTOI > actualExitTOI || actualEntryTOI > maxTime) {
            return false;
        }
        // If entryTOI < 0 and no initial overlap (they are separated): means moving away, no collision.
        // if (actualEntryTOI < 0.f && (!bodyRect.intersects(platRect))) return false;
        if (actualEntryTOI < 0.f) return false; // Simplification: only positive time impacts considered "new" collisions

    }


    outCollisionEvent.time = actualEntryTOI; // This is a fraction of `displacement`
    // Determine collision axis based on which entry time was larger
    if (entryTime.x > entryTime.y) {
        outCollisionEvent.axis = 0; // Collision on X-axis
    } else {
        outCollisionEvent.axis = 1; // Collision on Y-axis
    }
    // For Y-axis, if entryTime.x == entryTime.y, prioritize Y to land on things properly.
    // Can be ambiguous for perfect corner hits.
    if (entryTime.x == entryTime.y && displacement.y !=0 ) { // Favor Y if moving in Y for landing
         outCollisionEvent.axis = 1;
    }


    // Store a const_cast pointer if PlatformBody itself is const in the calling loop
    // but it is not const in `platformBodies` vector for CollisionSystem.
    // Casting away const from hitPlatform parameter is not needed if you just need to read from it.
    // The member `CollisionEvent::hitPlatform` should be `const PlatformBody*` if it always points to a const platform.
    // However, `platform` parameter to sweptAABB is already const, so this is fine:
    outCollisionEvent.hitPlatform = const_cast<PlatformBody*>(&platform); // Allow storing a non-const ptr if necessary
                                                                          // Ideally, if CollisionEvent only stores for read, make it const PlatformBody*

    return true;
}


void CollisionSystem::applyCollisionResponse(
    DynamicBody& dynamicBody,           // Non-const ref
    const CollisionEvent& event,
    const PlatformBody& hitPlatform)    // Const ref to platform hit
{
    sf::Vector2f vel = dynamicBody.getVelocity(); // Get current velocity

    if (event.axis == 0) { // Horizontal collision
        vel.x = 0.f;
    } else if (event.axis == 1) { // Vertical collision
        vel.y = 0.f;

        // Example: Conveyor Belt and Jump-through special logic
        // This should use `hitPlatform.getType()` and `hitPlatform.getSurfaceVelocity()`
        // Note: Jump-through downward pass-through is handled by NOT creating a collision event
        // in the first place (in `resolveCollisions` loop before calling `sweptAABB` or
        // by having `sweptAABB` return false for that specific interaction type).
        // Landing on jump-through IS a normal collision.
        if (hitPlatform.getType() == bodyType::conveyorBelt && vel.y == 0.f /*just landed*/) {
            // This effect is often better applied in the main game loop after all collisions resolved for frame,
            // using resolutionInfo.surfaceVelocity
        }
    }
    dynamicBody.setVelocity(vel); // Set modified velocity
}

} // namespace phys