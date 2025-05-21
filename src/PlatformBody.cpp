#include "PlatformBody.hpp"
#include "PhysicsTypes.hpp" 

namespace phys {

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
      m_velocity({0.f, 0.f}),
      m_width(width),
      m_height(height),
      m_type(type),
      m_falling(initiallyFalling),
      m_surfaceVelocity(surfaceVelocity) {}

void PlatformBody::update(float deltaTime) {
    if (m_falling && m_type == bodyType::falling) { // Example for self-managed falling

    }
}

sf::FloatRect PlatformBody::getAABB() const {
    return sf::FloatRect(m_position.x, m_position.y, m_width, m_height);
}

} // namespace phys