#include "tile.hpp" // Assuming Tile.hpp is the header name
#include "PhysicsTypes.hpp" 
Tile::Tile(const sf::Vector2f& size, const sf::Color& color) :
    m_shape(size),
    m_isFalling(false),
    m_fallDelayTimer(sf::Time::Zero),
    m_hasFallen(false),
    m_fallSpeed(200.f) // Example fall speed
{
    m_shape.setFillColor(color);
    // m_shape.setOrigin(size.x / 2.f, size.y / 2.f); // If you want origin at center
}

void Tile::update(sf::Time deltaTime) {
    if (m_hasFallen) return; // Nothing to do if already fallen

    if (!m_isFalling && m_fallDelayTimer > sf::Time::Zero) {
        m_fallDelayTimer -= deltaTime;
        if (m_fallDelayTimer <= sf::Time::Zero) {
            m_isFalling = true;
            // Optional: some visual cue it's about to fall or is falling
        }
    }

    if (m_isFalling) {
        move(0.f, m_fallSpeed * deltaTime.asSeconds()); // Move uses sf::Transformable's method
        // Example: Check if fallen off screen
        // if (getPosition().y > SOME_SCREEN_HEIGHT_LIMIT) {
        //     m_isFalling = false;
        //     m_hasFallen = true;
        //     // Optional: disable it, mark for removal, etc.
        // }
    }
}

void Tile::startFalling(sf::Time delay) {
    if (!m_isFalling && !m_hasFallen) { // Can only start falling if not already doing so or fallen
        m_fallDelayTimer = delay;
        if (delay <= sf::Time::Zero) { // If no delay, start falling immediately
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
    states.transform *= getTransform(); // Apply Tile's own transform (from sf::Transformable)
    target.draw(m_shape, states);
}