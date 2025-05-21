#include "Player.hpp"
#include "PlatformBody.hpp"

namespace phys {

DynamicBody::DynamicBody(
    const sf::Vector2f& initialPosition,
    float width,
    float height,
    const sf::Vector2f& initialVelocity)
    : m_position(initialPosition),
      m_velocity(initialVelocity),
      m_lastPosition(initialPosition),
      m_width(width),
      m_height(height),
      m_onGround(false),
      m_groundPlatform(nullptr),
      m_isTryingToDrop(false),
      m_tempIgnoredPlatform(nullptr)
{
}

sf::FloatRect DynamicBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

sf::FloatRect DynamicBody::getLastAABB() const {
    return sf::FloatRect(m_lastPosition.x, m_lastPosition.y, m_width, m_height);
}

void DynamicBody::setPosition(const sf::Vector2f& position) {
    m_position = position;
}

void DynamicBody::setVelocity(const sf::Vector2f& velocity) {
    m_velocity = velocity;
}

void DynamicBody::addVelocity(const sf::Vector2f& deltaVelocity) {
    m_velocity += deltaVelocity;
}

void DynamicBody::setLastPosition(const sf::Vector2f& position) {
    m_lastPosition = position;
}

// --- Specific Platform Interaction Logic Implementation ---
void DynamicBody::setGroundPlatform(const PlatformBody* platform) {
    m_groundPlatform = platform;
}

const PlatformBody* DynamicBody::getGroundPlatform() const {
    return m_groundPlatform;
}

void DynamicBody::setTryingToDrop(bool trying) {
    m_isTryingToDrop = trying;
}

bool DynamicBody::isTryingToDropFromPlatform() const {
    return m_isTryingToDrop;
}

void DynamicBody::setGroundPlatformTemporarilyIgnored(const PlatformBody* platform) {
    m_tempIgnoredPlatform = platform;
}

const PlatformBody* DynamicBody::getGroundPlatformTemporarilyIgnored() const {
    return m_tempIgnoredPlatform;
}

} // namespace phys