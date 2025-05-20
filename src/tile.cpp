#include "Tile.hpp"
const float FALLEN_Y_LIMIT = 2000.f;

Tile::Tile(const sf::Vector2f& size, const sf::Color& color)
    : m_shape(size),
      m_isFalling(false),
      m_fallDelayTimer(sf::Time::Zero),
      m_hasFallen(false),
      m_fallSpeed(200.f) {
    m_shape.setFillColor(color);
}

void Tile::update(sf::Time deltaTime) {
    if (m_hasFallen) {
        return; 
    }


    if (!m_isFalling && m_fallDelayTimer > sf::Time::Zero) {
        m_fallDelayTimer -= deltaTime;
        if (m_fallDelayTimer <= sf::Time::Zero) {
            m_isFalling = true;
            m_fallDelayTimer = sf::Time::Zero;
        }
    }

    // Handle falling movement
    if (m_isFalling) {
        this->move({0.f, m_fallSpeed * deltaTime.asSeconds()});

        if (getPosition().y > FALLEN_Y_LIMIT) {
            m_hasFallen = true;  // Mark as fallen
            m_isFalling = false; // Stop
        }
    }
}

void Tile::startFalling(sf::Time delay) {
    if (!m_isFalling && !m_hasFallen) {
        m_fallDelayTimer = delay;

        if (delay <= sf::Time::Zero) {
            m_isFalling = true;
            m_fallDelayTimer = sf::Time::Zero; // Reset timer
        }
    }
}

sf::FloatRect Tile::getGlobalBounds() const {
    if (m_hasFallen) {

        return sf::FloatRect();
    }

    return getTransform().transformRect(m_shape.getLocalBounds());
}

sf::FloatRect Tile::getLocalBounds() const {
    return m_shape.getLocalBounds();
}

void Tile::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (m_hasFallen) {
        return; // Don't draw if it has fallen
    }
    states.transform *= getTransform(); // Apply the Tile's own transform
    target.draw(m_shape, states);
}
