#include "PlatformBody.hpp"
#include "PhysicsTypes.hpp" 

namespace phys {

const sprite::Sprite DEFAULT_SPRITE("../assets/sprites/default.png");

PlatformBody::PlatformBody(
    unsigned int id,
    const sf::Vector2f& position,
    float width,
    float height,
    bodyType type,
    bool initiallyFalling,
    const sf::Vector2f& surfaceVelocity,
    const std::string texturePath)
    : m_id(id),
      m_position(position),
      m_velocity({0.f, 0.f}),
      m_width(width),
      m_height(height),
      m_type(type),
      m_falling(initiallyFalling),
      m_surfaceVelocity(surfaceVelocity),
      m_texturePath(texturePath) {}

void PlatformBody::update(float deltaTime) {
    if (m_falling && m_type == bodyType::falling) { // Example for self-managed falling

    }
}

sf::FloatRect PlatformBody::getAABB() const {
    return sf::FloatRect({m_position.x, m_position.y}, {m_width, m_height});
}

} // namespace phys