#include "dynamicBody.hpp"
#include "PhysicsTypes.hpp" 


namespace phys {

DynamicBody::DynamicBody(
    const sf::Vector2f& initialPosition,
    float width,
    float height,
    const sf::Vector2f& initialVelocity)
    : m_position(initialPosition),
      m_velocity(initialVelocity),
      m_lastPosition(initialPosition),
      m_inputDirection({0.f, 0.f}),
      m_width(width),
      m_height(height),
      m_onGround(false)
{
    // Constructor body (if any more logic is needed)
}

sf::FloatRect DynamicBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
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

// --- Placeholder Implementations for other methods ---
void DynamicBody::update(float deltaTime) {
    // m_lastPosition = m_position; // Should be set by the caller right before physics calc
    // m_position += m_velocity * deltaTime; // Actual position update happens in main physics loop
}

void DynamicBody::applyGravity(const sf::Vector2f& gravity, float deltaTime) {
    if (!m_onGround) { // m_onGround should be updated based on collision results
        m_velocity += gravity * deltaTime;
    }
}

void DynamicBody::handleInput(const sf::Vector2f& inputDirection) {
    m_inputDirection = inputDirection;
    // This method's purpose is for the DynamicBody to store input intention.
    // The main game loop would call this, then later DynamicBody::update or the main
    // loop's physics would use m_inputDirection to affect m_velocity.
}

} // namespace phys