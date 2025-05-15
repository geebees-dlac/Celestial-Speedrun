#ifndef TILE_HPP
#define TILE_HPP

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/RenderTarget.hpp> // For custom draw
#include <SFML/Graphics/RenderStates.hpp> // For custom draw
#include "PhysicsTypes.hpp" 
// Option 1: Inheriting sf::RectangleShape (Simpler for direct drawing if desired)
/*
class Tile : public sf::RectangleShape {
public:
    Tile(const sf::Vector2f& size = {32.f, 32.f}, const sf::Vector2f& position = {0.f, 0.f});
    // ~Tile() = default; // If destructor does nothing specific

    void update(sf::Time deltaTime); // To handle falling logic

    void startFalling(sf::Time delay = sf::Time::Zero);
    bool isFalling() const { return m_isFalling; }
    bool hasFallenOffScreen() const { return m_hasFallenOffScreen; } // Example

private:
    bool m_isFalling;
    sf::Time m_timeUntilFall;      // If there's a delay before falling starts
    sf::Vector2f m_fallVelocity;   // How fast it falls
    bool m_hasFallenOffScreen; // If you want to track this
};
*/

// Option 2: Composition (Often more flexible, Tile is its own Drawable)
class Tile : public sf::Drawable, public sf::Transformable {
public:
    Tile(const sf::Vector2f& size = {32.f, 32.f},
         const sf::Color& color = sf::Color::White);
    // ~Tile() = default;

    void update(sf::Time deltaTime);

    void startFalling(sf::Time delay = sf::Time::Zero);
    bool isFalling() const { return m_isFalling; }
    // bool hasFallen() const { return m_hasFallen; } // Or similar state

    // Expose only necessary parts of RectangleShape or provide own methods
    void setFillColor(const sf::Color& color) { m_shape.setFillColor(color); }
    const sf::Color& getFillColor() const { return m_shape.getFillColor(); }
    void setTexture(const sf::Texture* texture, bool resetRect = false) { m_shape.setTexture(texture, resetRect); }
    // setPosition, setSize, etc., are inherited from sf::Transformable if using this option

    sf::FloatRect getGlobalBounds() const;
    sf::FloatRect getLocalBounds() const;

private:
    // sf::Transformable provides getPosition, setPosition, getScale, setScale, etc.
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::RectangleShape m_shape;

    bool m_isFalling;
    sf::Time m_fallDelayTimer;    // Countdown before falling begins
    bool m_hasFallen;         // To indicate it has completed its fall sequence
    float m_fallSpeed;        // Pixels per second

    // Potentially other properties like type, breakable, etc.
};


#endif // TILE_HPP