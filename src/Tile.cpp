#include "Tile.hpp"
const float FALLEN_Y_LIMIT = 2000.f;

Tile::Tile(const sf::Vector2f& size, const sf::Color& color)
    : m_shape(size),
      m_isFalling(false),
      m_fallDelayTimer(sf::Time::Zero),
      m_hasFallen(false),
      m_fallSpeed(200.f) 
      {
    m_shape.setFillColor(color);
    m_shape.setSize(size);
}

void Tile::update(sf::Time deltaTime) {
    if (m_fallDelayTimer > sf::Time::Zero) {
        m_fallDelayTimer -= deltaTime;
        if (m_fallDelayTimer <= sf::Time::Zero) {
            m_isFalling = true;
        }
    }

    
    if (m_isFalling && !m_hasFallen) {
        float dy = m_fallSpeed * deltaTime.asSeconds();
        move(0.f, dy); 

        
        if (getPosition().y > 600.f) { 
            m_hasFallen = true;
            m_isFalling = false;
        }
    }
}

void Tile::startFalling(sf::Time delay) {
    if (!m_isFalling && !m_hasFallen && m_fallDelayTimer == sf::Time::Zero) {
        m_fallDelayTimer = delay;
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

