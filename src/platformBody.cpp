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

