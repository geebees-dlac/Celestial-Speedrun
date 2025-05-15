#include "platformBody.hpp"
#include "PhysicsTypes.hpp" 

namespace phys { // Namespace

PlatformBody::PlatformBody(
    unsigned int id, const sf::Vector2f& position, float width, float height,
    bodyType type, bool initiallyFalling, const sf::Vector2f& surfaceVelocity)
    : m_id(id),
      m_position(position),
      m_velocity({0.f, 0.f}), // Initialize if present
      m_width(width),
      m_height(height),
      m_type(type),
      m_falling(initiallyFalling),
      m_surfaceVelocity(surfaceVelocity) {}

sf::FloatRect PlatformBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

void PlatformBody::update(float deltaTime) {
    // Implement specific update logic if needed, e.g., for a platform that moves itself
    // if (m_type == bodyType::moving && m_isPlatformInternallyDriven) {
    //     m_position += m_internalVelocity * deltaTime;
    // }
}

} // namespace phys


//Just filler for me to finish tonight


/* namespace phys {

// Define static const members if any (example)
// const sf::Vector2f PlatformBody::GRAVITY = {0.f, 981.f};
phys::PlatformBody::PlatformBody(
    unsigned int id, const sf::Vector2f& position, float width, float height,
    bodyType type, bool initiallyFalling, const sf::Vector2f& surfaceVelocity)
    : m_id(id), m_position(position), m_width(width), m_height(height),
      m_type(type), m_falling(initiallyFalling), m_surfaceVelocity(surfaceVelocity),
      m_velocity({0.f, 0.f}) // Initialize m_velocity too
{
    // Any other setup
}
// Implement getAABB() and update(float) if needed
sf::FloatRect phys::PlatformBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

void phys::PlatformBody::update(float deltaTime) { /* ...  }
PlatformBody::PlatformBody(
    unsigned int id,
    const sf::Vector2f& position,
    float width,
    float height,
    bodyType type,
    bool initiallyFalling,
    const sf::Vector2f& surfaceVelocity)
    : m_id(id),
      m_position(position),
      m_velocity({0.f, 0.f}), // Initialize if used
      m_width(width),
      m_height(height),
      m_type(type),
      m_falling(initiallyFalling),
      m_surfaceVelocity(surfaceVelocity)
      // m_vanishTimer(type == bodyType::vanishing ? DEFAULT_VANISH_TIME : -1.f)
{
}

void PlatformBody::update(float deltaTime) {
    if (m_falling) {
        // Simple gravity example, assuming GRAVITY is defined
        // m_velocity += GRAVITY * deltaTime;
        // m_position += m_velocity * deltaTime;
    }

    if (m_type == bodyType::moving) {
        // Logic for programmed movement paths, oscillations, etc.
        // Example: m_position.x += m_surfaceVelocity.x * deltaTime; (if surfaceVel is its own motion)
    }

    // if (m_type == bodyType::vanishing && m_vanishTimer > 0.f) {
    //     m_vanishTimer -= deltaTime;
    //     if (m_vanishTimer <= 0.f) {
    //         // Mark for removal or disable
    //     }
    // }
}

sf::FloatRect PlatformBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

} // namespace phys */