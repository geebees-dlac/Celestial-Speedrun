// In CollisionSystem.cpp
#include "CollisionSystem.hpp"
// ... other necessary includes ...
#include "PhysicsTypes.hpp" 
namespace phys {

CollisionResolutionInfo CollisionSystem::resolveCollisions(
    DynamicBody& dynamicBody,
    const std::vector<PlatformBody>& platformBodies,
    float deltaTime)
{
    CollisionResolutionInfo resolutionInfo;
    sf::Vector2f originalVelocity = dynamicBody.getVelocity(); // Assuming dynamicBody has get/set Velocity
    sf::Vector2f movementThisFrame = originalVelocity * deltaTime;

    float timeRemaining = 1.0f; // Fraction of deltaTime remaining

    // It's often better to do multiple collision passes if high speeds are involved
    // or for more robust handling of multiple simultaneous contacts.
    // For simplicity, this is a common iterative approach for one frame:
    const int MAX_COLLISION_ITERATIONS = 5; // Prevent infinite loops

    for (int iter = 0; iter < MAX_COLLISION_ITERATIONS && timeRemaining > 0.0001f; ++iter) {
        float earliestCollisionTime = timeRemaining;
        CollisionEvent nearestCollision;
        nearestCollision.time = timeRemaining; // Initialize with max possible time
        PlatformBody const* hitPlatform = nullptr;

        sf::Vector2f currentVelocityForSweep = dynamicBody.getVelocity(); // May change after a response
        sf::Vector2f sweepVector = currentVelocityForSweep * (deltaTime * timeRemaining);

        // 1. Broadphase (skipped for simplicity, but crucial for many objects)
        //    For now, we check against all platforms (Narrowphase)

        // 2. Narrowphase: Find the earliest collision
        for (const auto& platform : platformBodies) {
            if (!dynamicBody.getAABB().intersects(platform.getAABB().getExpandedBy(sweepVector))) { // Simple pre-check
                 continue;
            }

            CollisionEvent currentEvent;
            if (sweptAABB(dynamicBody, currentVelocityForSweep, platform, earliestCollisionTime, currentEvent)) {
                if (currentEvent.time < earliestCollisionTime) {
                    earliestCollisionTime = currentEvent.time;
                    nearestCollision = currentEvent;
                    hitPlatform = &platform; // Store pointer to the platform we hit
                }
            }
        }

        // 3. Resolve the earliest collision
        if (hitPlatform) { // A collision was found in this iteration
            // Move body to point of impact
            dynamicBody.move(currentVelocityForSweep * deltaTime * earliestCollisionTime);

            // Apply collision response (adjust velocity, set flags)
            applyCollisionResponse(dynamicBody, nearestCollision, *hitPlatform);

            // Update resolutionInfo based on nearestCollision and hitPlatform
            if (nearestCollision.axis == 1) { // Y-axis collision
                if (dynamicBody.getVelocity().y > -0.001f && currentVelocityForSweep.y < 0) { // Moving down, now stopped/moving up slightly
                     resolutionInfo.onGround = true;
                     resolutionInfo.groundPlatform = hitPlatform; // For platform-specific interactions
                     // Example: if (hitPlatform->getType() == phys::BodyType::conveyorBelt) {
                     //    resolutionInfo.surfaceVelocity.x = hitPlatform->getSurfaceVelocity();
                     // }
                } else if (currentVelocityForSweep.y > 0) {
                     resolutionInfo.hitCeiling = true;
                }
            } else if (nearestCollision.axis == 0) { // X-axis collision
                // Similarly set hitWallLeft/Right
                 if (currentVelocityForSweep.x < 0) resolutionInfo.hitWallLeft = true;
                 else resolutionInfo.hitWallRight = true;
            }


            timeRemaining -= earliestCollisionTime; // Reduce remaining time by the time consumed
        } else {
            // No collision found in the remaining time
            dynamicBody.move(currentVelocityForSweep * deltaTime * timeRemaining);
            timeRemaining = 0; // Done
        }
    }
    if (timeRemaining > 0.0001f && iter == MAX_COLLISION_ITERATIONS) {
        // Potentially stuck or too many collisions, handle or log
        // For safety, might move by remaining velocity or stop
        dynamicBody.move(dynamicBody.getVelocity() * deltaTime * timeRemaining);
    }


    // Update dynamicBody's internal collision flags if it has them based on resolutionInfo
    dynamicBody.m_collisionBottom = resolutionInfo.onGround;
    dynamicBody.m_collisionTop = resolutionInfo.hitCeiling;
    // ... etc.
    // dynamicBody.m_surfaceVelocity = resolutionInfo.surfaceVelocity;


    return resolutionInfo;
}


bool CollisionSystem::sweptAABB(
    const DynamicBody& body,
    const sf::Vector2f& velocityPerTimestep, // This is dBody.vel * dt (total movement for the *full* timestep)
    const PlatformBody& platform,
    float maxTime, // This is the fraction of the *remaining* timestep we are allowed to collide in
    CollisionEvent& outCollisionEvent)
{
    // Based on https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/swept-aabb-collision-detection-and-response-r3084/
    // But we receive velocityPerTimestep = body.velocity * deltaTime,
    // and maxTime is the fraction of this *deltaTime* we're still considering.
    // The sweep vector is therefore velocityPerTimestep * maxTime.

    sf::FloatRect bodyRect = body.getAABB(); // Get current AABB
    sf::FloatRect platRect = platform.getAABB();

    // If already overlapping (or very close), treat as immediate collision (time = 0)
    // This needs careful handling, e.g., depenetration first, or special logic
    // For now, we assume they are not initially overlapping deeply.
    // A small overlap might be fine if your resolution pushes them apart.
    if (bodyRect.intersects(platRect)) {
        // This is tricky. A simple 'time = 0' might not be right if not perfectly aligned.
        // It depends on how you want to handle initial overlaps.
        // Often, one would resolve existing penetration first, then sweep.
        // For now, let's assume sweep is for finding *new* penetrations.
        // outCollisionEvent.time = 0.0f;
        // // Determine axis based on overlap or a default? This part is complex for initial overlap.
        // outCollisionEvent.axis = ... ;
        // return true;
        // Or simply ignore if initial check (maxTime == 1.0f) as it might be a previous frame's residual.
        // For simplicity in this example, we will just proceed with the sweep.
    }

    sf::Vector2f invEntry, invExit;
    sf::Vector2f entryTime, exitTime;

    // Find the distance between the objects on the near and far sides for both x and y
    // X-axis
    if (velocityPerTimestep.x > 0.0f) {
        invEntry.x = platRect.left - (bodyRect.left + bodyRect.width);
        invExit.x  = (platRect.left + platRect.width) - bodyRect.left;
    } else {
        invEntry.x = (platRect.left + platRect.width) - bodyRect.left;
        invExit.x  = platRect.left - (bodyRect.left + bodyRect.width);
    }

    // Y-axis
    if (velocityPerTimestep.y > 0.0f) {
        invEntry.y = platRect.top - (bodyRect.top + bodyRect.height);
        invExit.y  = (platRect.top + platRect.height) - bodyRect.top;
    } else {
        invEntry.y = (platRect.top + platRect.height) - bodyRect.top;
        invExit.y  = platRect.top - (bodyRect.top + bodyRect.height);
    }

    // Find time of collision and time of leaving for each axis (if statement is to prevent divide by zero)
    if (velocityPerTimestep.x == 0.0f) {
        entryTime.x = -std::numeric_limits<float>::infinity(); // Should not hit if not moving towards
        exitTime.x  = std::numeric_limits<float>::infinity();
        if (bodyRect.left + bodyRect.width <= platRect.left || bodyRect.left >= platRect.left + platRect.width) {
             // If not moving in X and not overlapping in X, no X collision possible
             return false;
        }
    } else {
        entryTime.x = invEntry.x / velocityPerTimestep.x;
        exitTime.x  = invExit.x / velocityPerTimestep.x;
    }

    if (velocityPerTimestep.y == 0.0f) {
        entryTime.y = -std::numeric_limits<float>::infinity();
        exitTime.y  = std::numeric_limits<float>::infinity();
         if (bodyRect.top + bodyRect.height <= platRect.top || bodyRect.top >= platRect.top + platRect.height) {
            // If not moving in Y and not overlapping in Y, no Y collision possible
            return false;
        }
    } else {
        entryTime.y = invEntry.y / velocityPerTimestep.y;
        exitTime.y  = invExit.y / velocityPerTimestep.y;
    }

    // Find the earliest/latest times of collision
    float actualEntryTime = std::max(entryTime.x, entryTime.y);
    float actualExitTime = std::min(exitTime.x, exitTime.y);

    // If no collision
    if (actualEntryTime > actualExitTime ||         // No overlap in time intervals
        entryTime.x < 0.0f && entryTime.y < 0.0f || // Moving away from object
        entryTime.x > maxTime || entryTime.y > maxTime) { // Collision occurs after allowed time
        // The condition `entryTime.x > 1.0f || entryTime.y > 1.0f` from standard swept AABB
        // becomes `entryTime.x > maxTime || entryTime.y > maxTime` if maxTime < 1
        // More robustly, actualEntryTime must be within [0, maxTime]
        if (actualEntryTime < 0.0f || actualEntryTime > maxTime) {
            return false;
        }
    }


    // If collision, calculate normal of collision
    outCollisionEvent.time = actualEntryTime;
    if (entryTime.x > entryTime.y) {
        outCollisionEvent.axis = 0; // Collision on X-axis
        // Further determine if it's left or right wall based on velocityPerTimestep.x
        // if (invEntry.x < 0.0f) normalx = 1.0f; else normalx = -1.0f;
    } else {
        outCollisionEvent.axis = 1; // Collision on Y-axis
        // if (invEntry.y < 0.0f) normaly = 1.0f; else normaly = -1.0f;
    }
    outCollisionEvent.hitPlatform = &platform; // Could be useful for response

    return true;
}


void CollisionSystem::applyCollisionResponse(
    DynamicBody& dynamicBody,
    const CollisionEvent& event,
    const PlatformBody& hitPlatform)
{
    // Simple slide response for AABB
    sf::Vector2f vel = dynamicBody.getVelocity();
    if (event.axis == 0) { // Collision on X axis
        vel.x = 0;
        // Optional: Apply friction or bounce if desired
        // vel.y *= frictionCoefficient;
    } else if (event.axis == 1) { // Collision on Y axis
        vel.y = 0;
        // Example: Conveyor belt interaction (assumes PlatformBody has relevant methods)
        // if (hitPlatform.getType() == phys::BodyType::conveyorBelt) {
        //     vel.x += hitPlatform.getSurfaceInfluence().x; // Or just set it
        // }
        // if (hitPlatform.getType() == phys::BodyType::jumpthrough && vel.y > 0 /*falling onto it*/) {
        //    // allow fall through if player was moving up through it last frame.
        //    // This logic can be complex, usually managed by checking relative positions + velocity
        // }
    }
    dynamicBody.setVelocity(vel);

    // Update internal flags on dynamicBody based on the collision side
    // (This part can be tricky; often done after all iterations in resolveCollisions)
    // Example:
    // if (event.axis == 1 && original_velocity.y < 0) dynamicBody.setOnGround(true);
    // if (event.axis == 1 && original_velocity.y > 0) dynamicBody.setHitCeiling(true);
    // ... etc.
}


} // namespace phys