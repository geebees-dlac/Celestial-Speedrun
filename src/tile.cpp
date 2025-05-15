#include "Tile.hpp" // Use ""

// If Tile.hpp uses namespace phys for the Tile class, then this should too
// Assuming Tile class is global scope as per Tile.hpp provided

Tile::Tile(const sf::Vector2f& size, const sf::Color& color)
    : m_shape(size),
      m_isFalling(false),
      m_fallDelayTimer(sf::Time::Zero),
      m_hasFallen(false),
      m_fallSpeed(200.f) {
    m_shape.setFillColor(color);
    // For sf::Transformable part, position will be set via setPosition()
}

void Tile::update(sf::Time deltaTime) {
    if (m_hasFallen) return;

    if (!m_isFalling && m_fallDelayTimer > sf::Time::Zero) {
        m_fallDelayTimer -= deltaTime;
        if (m_fallDelayTimer <= sf::Time::Zero) {
            m_isFalling = true;
        }
    }

    if (m_isFalling) {
        // sf::Transformable::move is inherited
        this->move(0.f, m_fallSpeed * deltaTime.asSeconds());
    }
}

void Tile::startFalling(sf::Time delay) {
    if (!m_isFalling && !m_hasFallen) {
        m_fallDelayTimer = delay;
        if (delay <= sf::Time::Zero) {
            m_isFalling = true;
        }
    }
}

sf::FloatRect Tile::getGlobalBounds() const {
    return getTransform().transformRect(m_shape.getLocalBounds());
}

sf::FloatRect Tile::getLocalBounds() const {
    return m_shape.getLocalBounds();
}

void Tile::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform(); // Apply the Tile's own transform
    target.draw(m_shape, states);
}