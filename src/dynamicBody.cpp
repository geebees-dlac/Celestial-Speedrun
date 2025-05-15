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
// namespace phys

// UPDATE LOGIC FROM BELOW I IMPORT FROM THE PDF TO THINK ABOUT

/* phys::DynamicBody::DynamicBody(
    const sf::Vector2f& initialPosition, float width, float height, const sf::Vector2f& initialVelocity)
    : m_position(initialPosition), m_velocity(initialVelocity), m_lastPosition(initialPosition),
      m_inputDirection({0.f,0.f}), m_width(width), m_height(height), m_onGround(false)
{
    // Any other setup
}
// Implement getAABB(), update(), setPosition(), setVelocity() etc.
sf::FloatRect phys::DynamicBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}
void phys::DynamicBody::update(float deltaTime) { Placeholder }
void phys::DynamicBody::applyGravity(const sf::Vector2f& gravity, float deltaTime) { Placeholder }
void phys::DynamicBody::handleInput(const sf::Vector2f& inputDirection) { Placeholder }

void phys::DynamicBody::setPosition(const sf::Vector2f& position) { m_position = position; }
void phys::DynamicBody::setVelocity(const sf::Vector2f& velocity) { m_velocity = velocity; }
void phys::DynamicBody::addVelocity(const sf::Vector2f& deltaVelocity) { m_velocity += deltaVelocity; }
namespace phys {

DynamicBody::DynamicBody(
    const sf::Vector2f& initialPosition,
    float width,
    float height,
    const sf::Vector2f& initialVelocity)
    : m_position(initialPosition),
      m_velocity(initialVelocity),
      m_lastPosition(initialPosition), // Initialize lastPosition too
      m_inputDirection({0.f, 0.f}),
      m_width(width),
      m_height(height),
      m_onGround(false)
      // Initialize other members if any
{
}

/*void DynamicBody::update(float deltaTime /*, const CollisionResolutionInfo& collisions ) {
    m_lastPosition = m_position;

    // Basic Euler integration (can be improved with Verlet, RK4, etc. if needed)
    // m_velocity += m_acceleration * deltaTime; // If you have acceleration term
    m_position += m_velocity * deltaTime;

    // Example: Use m_inputDirection to affect velocity
    // float accelerationRate = 500.f; // Could be a member
    // m_velocity.x += m_inputDirection.x * accelerationRate * deltaTime;
    // m_velocity.y += m_inputDirection.y * accelerationRate * deltaTime; // If input affects Y

    // Logic based on collision flags (if used)
    // if (m_onGround && m_inputDirection.y < 0) { /* Jump logic  }

    // Reset input after processing if it's a per-frame thing
    // m_inputDirection = {0.f, 0.f};
}

/*void DynamicBody::applyGravity(const sf::Vector2f& gravity, float deltaTime) {
    if (!m_onGround) { // Only apply gravity if not on ground
        m_velocity += gravity * deltaTime;
    }
}

void DynamicBody::handleInput(const sf::Vector2f& inputDirection) {
    m_inputDirection = inputDirection;
    // Or directly apply to velocity if input is more direct force/acceleration
    // E.g. m_velocity.x = inputDirection.x * MAX_PLAYER_SPEED;
}


sf::FloatRect DynamicBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

void DynamicBody::setPosition(const sf::Vector2f& position) {
    m_position = position;
    // Consider if m_lastPosition should also be updated here or only in update()
}

void DynamicBody::setVelocity(const sf::Vector2f& velocity) {
    m_velocity = velocity;
}

void DynamicBody::addVelocity(const sf::Vector2f& deltaVelocity) {
    m_velocity += deltaVelocity;
}


} // namespace phys */
